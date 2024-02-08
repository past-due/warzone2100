
/* Setup persistent config dir */
function wzSetupPersistentConfigDir() {

	Module['WZVAL_configDirSuffix'] = '';
	if (typeof wz_js_get_config_dir_suffix === "function") {
		Module['WZVAL_configDirSuffix'] = wz_js_get_config_dir_suffix();
	}
	else {
		console.log('Unable to get config dir suffix');
	}

	let configDirPath = '/warzone2100' + Module['WZVAL_configDirSuffix'];

	// Create a directory in IDBFS to store the config dir
	FS.mkdir(configDirPath);

	// Mount IDBFS as the file system
	FS.mount(FS.filesystems.IDBFS, {}, configDirPath);

	// Synchronize IDBFS -> Emscripten virtual filesystem
	Module["addRunDependency"]("persistent_warzone2100_config_dir");
	FS.syncfs(true, (err) => {
		console.log(FS.readdir(configDirPath));
		Module["removeRunDependency"]("persistent_warzone2100_config_dir");
	})
}
function wzSaveConfigDirToPersistentStore(callback) {
	let configDirPath = '/warzone2100' + Module['WZVAL_configDirSuffix'];
	FS.syncfs(false, (err) => {
		console.log('saved to idbfs', FS.readdir(configDirPath));
		if (callback) callback();
	})
}
Module.wzSaveConfigDirToPersistentStore = wzSaveConfigDirToPersistentStore;

if (!Module['preRun'])
{
	Module['preRun'] = [];
}
Module['preRun'].push(wzSetupPersistentConfigDir);

Module["onExit"] = function() {
	// Sync IDBFS
	wzSaveConfigDirToPersistentStore(() => {
		if (typeof wz_js_display_loading_indicator === "function") {
			wz_js_display_loading_indicator(false);
		}
		else {
			alert('It is now safe to close your browser window.');
		}
	});

	if (typeof wz_js_handle_app_exit === "function") {
		wz_js_handle_app_exit();
	}
}

Module['ASAN_OPTIONS'] = 'halt_on_error=0'
