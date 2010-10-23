drop table import_status;
CREATE TABLE import_status (
  lastimportdate timestamp NOT NULL
  );
GRANT SELECT ON import_status TO "www-data" ;

drop table import_osmosis_log;
CREATE TABLE import_osmosis_log (
  batchend timestamp,
  batchsize integer,
  starttime timestamp,
  endtime timestamp,
  event text
  );

--drop table IF EXISTS query_log;
CREATE TABLE query_log (
  starttime timestamp,
  query text,
  ipaddress text,
  endtime timestamp,
  results integer
  );
CREATE INDEX idx_query_log ON query_log USING BTREE (starttime);
GRANT INSERT ON query_log TO "www-data" ;

CREATE TABLE new_query_log (
  type text,
  starttime timestamp,
  ipaddress text,
  useragent text,
  language text,
  query text,
  endtime timestamp,
  results integer,
  format text,
  secret text
  );
CREATE INDEX idx_new_query_log_starttime ON new_query_log USING BTREE (starttime);
GRANT INSERT ON new_query_log TO "www-data" ;
GRANT UPDATE ON new_query_log TO "www-data" ;
GRANT SELECT ON new_query_log TO "www-data" ;

create view vw_search_query_log as SELECT substr(query, 1, 50) AS query, starttime, endtime - starttime AS duration, substr(useragent, 1, 20) as 
useragent, language, results, ipaddress FROM new_query_log WHERE type = 'search' ORDER BY starttime DESC;

--drop table IF EXISTS report_log;
CREATE TABLE report_log (
  starttime timestamp,
  ipaddress text,
  query text,
  description text,
  email text
  );
GRANT INSERT ON report_log TO "www-data" ;

drop table IF EXISTS word;
CREATE TABLE word (
  word_id INTEGER,
  word_token text,
  word_trigram text,
  word text,
  class text,
  type text,
  country_code varchar(2),
  search_name_count INTEGER
  );
SELECT AddGeometryColumn('word', 'location', 4326, 'GEOMETRY', 2);
CREATE INDEX idx_word_word_id on word USING BTREE (word_id);
CREATE INDEX idx_word_word_token on word USING BTREE (word_token);
CREATE INDEX idx_word_trigram ON word USING gin(word_trigram gin_trgm_ops);
GRANT SELECT ON word TO "www-data" ;
DROP SEQUENCE seq_word;
CREATE SEQUENCE seq_word start 1;

drop table IF EXISTS location_area CASCADE;
CREATE TABLE location_area (
  partition varchar(10),
  place_id bigint,
  keywords INTEGER[],
  rank_search INTEGER NOT NULL,
  rank_address INTEGER NOT NULL,
  isguess BOOL
  );
SELECT AddGeometryColumn('location_area', 'centroid', 4326, 'POINT', 2);
SELECT AddGeometryColumn('location_area', 'geometry', 4326, 'GEOMETRY', 2);

CREATE TABLE location_area_large () INHERITS (location_area);
CREATE TABLE location_area_roadnear () INHERITS (location_area);
CREATE TABLE location_area_roadfar () INHERITS (location_area);

drop table IF EXISTS search_name;
CREATE TABLE search_name (
  place_id bigint,
  search_rank integer,
  address_rank integer,
  country_code varchar(2),
  name_vector integer[],
  nameaddress_vector integer[]
  );
CREATE INDEX search_name_name_vector_idx ON search_name USING GIN (name_vector gin__int_ops);
CREATE INDEX searchnameplacesearch_search_nameaddress_vector_idx ON search_name USING GIN (nameaddress_vector gin__int_ops);
SELECT AddGeometryColumn('search_name', 'centroid', 4326, 'GEOMETRY', 2);
CREATE INDEX idx_search_name_centroid ON search_name USING GIST (centroid);
CREATE INDEX idx_search_name_place_id ON search_name USING BTREE (place_id);

drop table IF EXISTS place_addressline;
CREATE TABLE place_addressline (
  place_id bigint,
  address_place_id bigint,
  fromarea boolean,
  isaddress boolean,
  distance float,
  cached_rank_address integer
  );
CREATE INDEX idx_place_addressline_place_id on place_addressline USING BTREE (place_id);
CREATE INDEX idx_place_addressline_address_place_id on place_addressline USING BTREE (address_place_id);

drop table IF EXISTS place_boundingbox CASCADE;
CREATE TABLE place_boundingbox (
  place_id bigint,
  minlat float,
  maxlat float,
  minlon float,
  maxlon float,
  numfeatures integer,
  area float
  );
CREATE INDEX idx_place_boundingbox_place_id on place_boundingbox USING BTREE (place_id);
SELECT AddGeometryColumn('place_boundingbox', 'outline', 4326, 'GEOMETRY', 2);
CREATE INDEX idx_place_boundingbox_outline ON place_boundingbox USING GIST (outline);
GRANT SELECT on place_boundingbox to "www-data" ;
GRANT INSERT on place_boundingbox to "www-data" ;

drop table IF EXISTS reverse_cache;
CREATE TABLE reverse_cache (
  latlonzoomid integer,
  country_code varchar(2),
  place_id bigint
  );
GRANT SELECT on reverse_cache to "www-data" ;
GRANT INSERT on reverse_cache to "www-data" ;
CREATE INDEX idx_reverse_cache_latlonzoomid ON reverse_cache USING BTREE (latlonzoomid);

drop table country;
CREATE TABLE country (
  country_code varchar(2),
  country_name hstore,
  country_default_language_code varchar(2)
  );
SELECT AddGeometryColumn('country', 'geometry', 4326, 'POLYGON', 2);
insert into country select iso3166::varchar(2), ARRAY[ROW('name:en',cntry_name)::keyvalue], null, 
  ST_Transform(geometryn(the_geom, generate_series(1, numgeometries(the_geom))), 4326) from worldboundaries;
CREATE INDEX idx_country_country_code ON country USING BTREE (country_code);
CREATE INDEX idx_country_geometry ON country USING GIST (geometry);

drop table placex;
CREATE TABLE placex (
  place_id bigint NOT NULL,
  partition varchar(10),
  osm_type char(1),
  osm_id bigint,
  class TEXT NOT NULL,
  type TEXT NOT NULL,
  name HSTORE,
  admin_level integer,
  housenumber TEXT,
  street TEXT,
  isin TEXT,
  postcode TEXT,
  country_code varchar(2),
  street_place_id bigint,
  rank_address INTEGER,
  rank_search INTEGER,
  indexed_status INTEGER,
  indexed_date TIMESTAMP,
  geometry_sector INTEGER
  );
SELECT AddGeometryColumn('placex', 'geometry', 4326, 'GEOMETRY', 2);
CREATE UNIQUE INDEX idx_place_id ON placex USING BTREE (place_id);
CREATE INDEX idx_placex_osmid ON placex USING BTREE (osm_type, osm_id);
CREATE INDEX idx_placex_rank_search ON placex USING BTREE (rank_search);
CREATE INDEX idx_placex_rank_address ON placex USING BTREE (rank_address);
CREATE INDEX idx_placex_geometry ON placex USING GIST (geometry);
CREATE INDEX idx_placex_indexed ON placex USING BTREE (indexed);
CREATE INDEX idx_placex_pending ON placex USING BTREE (rank_search) where name IS NOT NULL and indexed = false;
CREATE INDEX idx_placex_pendingbylatlon ON placex USING BTREE (geometry_index(geometry_sector,indexed,name),rank_search) 
  where geometry_index(geometry_sector,indexed,name) IS NOT NULL;
CREATE INDEX idx_placex_street_place_id ON placex USING BTREE (street_place_id) where street_place_id IS NOT NULL;
CREATE INDEX idx_placex_gb_postcodesector ON placex USING BTREE (substring(upper(postcode) from '^([A-Z][A-Z]?[0-9][0-9A-Z]? [0-9])[A-Z][A-Z]$'))
  where country_code = 'gb' and substring(upper(postcode) from '^([A-Z][A-Z]?[0-9][0-9A-Z]? [0-9])[A-Z][A-Z]$') is not null;
CREATE INDEX idx_placex_interpolation ON placex USING BTREE (geometry_sector) where indexed = false and class='place' and type='houses';
CREATE INDEX idx_placex_sector ON placex USING BTREE (geometry_sector,rank_address,osm_type,osm_id);
CLUSTER placex USING idx_placex_sector;

DROP SEQUENCE seq_place;
CREATE SEQUENCE seq_place start 1;
GRANT SELECT on placex to "www-data" ;
GRANT UPDATE ON placex to "www-data" ;
GRANT SELECT ON search_name to "www-data" ;
GRANT DELETE on search_name to "www-data" ;
GRANT INSERT on search_name to "www-data" ;
GRANT SELECT on place_addressline to "www-data" ;
GRANT INSERT ON place_addressline to "www-data" ;
GRANT DELETE on place_addressline to "www-data" ;
GRANT SELECT on location_point to "www-data" ;
GRANT SELECT ON seq_word to "www-data" ;
GRANT UPDATE ON seq_word to "www-data" ;
GRANT INSERT ON word to "www-data" ;
GRANT SELECT ON planet_osm_ways to "www-data" ;
GRANT SELECT ON planet_osm_rels to "www-data" ;
GRANT SELECT on location_point to "www-data" ;
GRANT SELECT on location_area to "www-data" ;
GRANT SELECT on location_point_26 to "www-data" ;
GRANT SELECT on location_point_25 to "www-data" ;
GRANT SELECT on location_point_24 to "www-data" ;
GRANT SELECT on location_point_23 to "www-data" ;
GRANT SELECT on location_point_22 to "www-data" ;
GRANT SELECT on location_point_21 to "www-data" ;
GRANT SELECT on location_point_20 to "www-data" ;
GRANT SELECT on location_point_19 to "www-data" ;
GRANT SELECT on location_point_18 to "www-data" ;
GRANT SELECT on location_point_17 to "www-data" ;
GRANT SELECT on location_point_16 to "www-data" ;
GRANT SELECT on location_point_15 to "www-data" ;
GRANT SELECT on location_point_14 to "www-data" ;
GRANT SELECT on location_point_13 to "www-data" ;
GRANT SELECT on location_point_12 to "www-data" ;
GRANT SELECT on location_point_11 to "www-data" ;
GRANT SELECT on location_point_10 to "www-data" ;
GRANT SELECT on location_point_9 to "www-data" ;
GRANT SELECT on location_point_8 to "www-data" ;
GRANT SELECT on location_point_7 to "www-data" ;
GRANT SELECT on location_point_6 to "www-data" ;
GRANT SELECT on location_point_5 to "www-data" ;
GRANT SELECT on location_point_4 to "www-data" ;
GRANT SELECT on location_point_3 to "www-data" ;
GRANT SELECT on location_point_2 to "www-data" ;
GRANT SELECT on location_point_1 to "www-data" ;
GRANT SELECT on country to "www-data" ;

-- insert creates the location tagbles, creates location indexes if indexed == true
CREATE TRIGGER placex_before_insert BEFORE INSERT ON placex
    FOR EACH ROW EXECUTE PROCEDURE placex_insert();

-- update insert creates the location tables
CREATE TRIGGER placex_before_update BEFORE UPDATE ON placex
    FOR EACH ROW EXECUTE PROCEDURE placex_update();

-- diff update triggers
CREATE TRIGGER placex_before_delete AFTER DELETE ON placex
    FOR EACH ROW EXECUTE PROCEDURE placex_delete();
CREATE TRIGGER place_before_delete BEFORE DELETE ON place
    FOR EACH ROW EXECUTE PROCEDURE place_delete();
CREATE TRIGGER place_before_insert BEFORE INSERT ON place
    FOR EACH ROW EXECUTE PROCEDURE place_insert();

alter table placex add column geometry_sector INTEGER;
alter table placex add column indexed_status INTEGER;
alter table placex add column indexed_date TIMESTAMP;

update placex set geometry_sector = geometry_sector(geometry);
drop index idx_placex_pendingbylatlon;
drop index idx_placex_interpolation;
drop index idx_placex_sector;
CREATE INDEX idx_placex_pendingbylatlon ON placex USING BTREE (geometry_index(geometry_sector,indexed,name),rank_search) 
  where geometry_index(geometry_sector,indexed,name) IS NOT NULL;
CREATE INDEX idx_placex_interpolation ON placex USING BTREE (geometry_sector) where indexed = false and class='place' and type='houses';
CREATE INDEX idx_placex_sector ON placex USING BTREE (geometry_sector,rank_address,osm_type,osm_id);
