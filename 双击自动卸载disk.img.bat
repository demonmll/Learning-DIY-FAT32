set vhd_file=vhd.cfg
del %vhd_file%
echo SELECT VDISK FILE="%cd%\disk.img" >> %vhd_file%
echo DETACH VDISK >> %vhd_file%
echo EXIT >> %vhd_file%

diskpart /s %vhd_file%

if %ERRORLEVEL% NEQ 0 (
   	echo "��������"
	pause
)