del /f c_lib.cp38-win_amd64.pyd
cd c_lib
python setup.py build
cd ..
xcopy /y c_lib\build\lib.win-amd64-3.8\c_lib.cp38-win_amd64.pyd .
del /f c_lib\build\lib.win-amd64-3.8\c_lib.cp38-win_amd64.pyd
PAUSE