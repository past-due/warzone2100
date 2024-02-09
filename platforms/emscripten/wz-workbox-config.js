// NOTE: This config should be run on the *installed* files
module.exports = {
	// -----------------------------
	// Pre-cache configuration
	globDirectory: './',
	globPatterns: [
		'index.html',
		'manifest.json',
		// warzone2100.js, warzone2100.worker.js
		'*.js',
		// Core WASM file
		'warzone2100.wasm',
		// Specific favicon assets
		'assets/favicon-16x16.png',
		'assets/favicon-32x32.png',
		'assets/android-chrome-192x192.png',
		// Any remaining css, js, or json files from the assets directory
		'assets/*.{css,js,json}'
	],
	globIgnores: [
		'**\/node_modules\/**\/*',
		'**/*.data', // do not precache .data files, which are often huge
		'**/*.debug.wasm', // do not precache wasm debug symbols
		'**\/music\/**\/*', // do not precache music (which is optional)
		'**\/terrain_overrides\/**\/*' // do not precache terrain_overrides (which are optional)
	],
	// NOTE: These should match the versions used in shell.html!
	additionalManifestEntries: [
		{ url: 'https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.2/css/bootstrap.min.css', revision: null, integrity: 'sha512-b2QcS5SsA8tZodcDtGRELiGv5SaKSk1vDHDaQRda0htPYWZ6046lr3kJ5bAAQdpV2mmA/4v0wQF9MyU6/pDIAg==' },
		{ url: 'https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.2/js/bootstrap.bundle.min.js', revision: null, integrity: 'sha512-X/YkDZyjTf4wyc2Vy16YGCPHwAY8rZJY+POgokZjQB2mhIRFJCckEGc6YyX9eNsPfn0PzThEuNs+uaomE5CO6A==' }
	],
	maximumFileSizeToCacheInBytes: 104857600, // Must be greater than the file size of any precached files
	// -----------------------------
	// Runtime caching configuration
	runtimeCaching: [
		// NOTE: warzone2100.data is already cached by the Emscripten preload cache, and is version-specific, so do not cache it below
		//
		// Cache music & terrain_overrides data loader JS
		{
			urlPattern: new RegExp('/(music|terrain_overrides)/.*\.js$'),
			handler: 'StaleWhileRevalidate',
			options: {
				cacheName: 'optional-data-js-loaders',
				expiration: {
					maxEntries: 10,
					purgeOnQuotaError: true
				}
			}
		},
		// Cache music & terrain_overrides data for offline use
		{
			urlPattern: new RegExp('/(music|terrain_overrides)/.*\.data$'),
			handler: 'NetworkFirst',
			options: {
				cacheName: 'optional-data-packages',
				expiration: {
					maxEntries: 10,
					purgeOnQuotaError: true
				}
			}
		},
		// Backup on-demand caching of any additional utilized CSS and JS files for offline use
		// (useful in case someone forgot to update the additionalManifestEntries above)
		{
			urlPattern: new RegExp('/.*\.(js|css)$'),
			handler: 'NetworkFirst',
			options: {
				cacheName: 'additional-dependencies',
				expiration: {
					maxEntries: 20,
					purgeOnQuotaError: true
				}
			}
		},
	],
	// -----------------------------
	swDest: './service-worker.js',
	offlineGoogleAnalytics: false,
	ignoreURLParametersMatching: [
		/^utm_/,
		/^fbclid$/
	]
};
