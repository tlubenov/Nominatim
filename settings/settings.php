<?php

	// General settings
	@define('CONST_Debug', false);
	@define('CONST_Database_DSN', 'pgsql://@/nominatim');

	// Website settings
	@define('CONST_ClosedForIndexing', false);
	@define('CONST_ClosedForIndexingExceptionIPs', '');
	@define('CONST_BlockedIPs', '');

	@define('CONST_Website_BaseURL', 'http://'.php_uname('n').'/');

	@define('CONST_Default_Language', 'xx');
	@define('CONST_Default_Lat', 20.0);
	@define('CONST_Default_Lon', 0.0);
	@define('CONST_Default_Zoom', 2);

	@define('CONST_Search_AreaPolygons_Enabled', true);

	@define('CONST_Suggestions_Enabled', false);


