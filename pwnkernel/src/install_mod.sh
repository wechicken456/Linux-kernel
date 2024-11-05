if [ -z "$1" ]; then
		echo "USAGE: sudo ./install_mod.sh MODULE_NAME"
else 
	MODULE_NAME=$1
	rmmod $MODULE_NAME 2>/dev/null
	/usr/src/linux-headers-6.8.0-45-generic/scripts/sign-file sha256 kernel_key.priv kernel_key.der $MODULE_NAME
	insmod $MODULE_NAME
fi
