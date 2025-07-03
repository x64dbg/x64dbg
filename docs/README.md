# docs

Documentation repository for [x64dbg](http://x64dbg.com) at [Read the Docs](https://readthedocs.org/projects/x64dbg).

## Building

1. Download https://github.com/x64dbg/docs/releases/download/python27-portable/python-2.7.18.amd64.portable.7z
2. Extract to the `python-2.7.18.amd64.portable` folder
3. run `makechm.bat`. It will build the .CHM help file.

Note: The following patch was applied:

Add `relpath = relpath.replace(os.path.sep, '/')` after `C:\Python27\Lib\site-packages\recommonmark\transform.py` line `63`