#!/bin/sh
# Build lsq_sizing

echo "Installing lsq_sizing"
make clean
make || sink "make"
rm $DHLS_INSTALL_DIR/bin/lsq_sizing
ln -s $DHLS_INSTALL_DIR/etc/dynamatic/lsq_sizing/bin/lsq_sizing $DHLS_INSTALL_DIR/bin/lsq_sizing
