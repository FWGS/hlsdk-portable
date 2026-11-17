#!/bin/bash

# DO NOT NEW ADD FILES WITHOUT A REASON
FILES_LIST=" \
	common/bspfile.h \
	common/cl_entity.h \
	common/const.h \
	common/con_nprint.h \
	common/com_model.h \
	common/cvardef.h \
	common/entity_state.h \
	common/entity_types.h \
	common/event_args.h \
	common/gameinfo.h \
	common/netadr.h \
	common/net_api.h \
	common/port.h \
	common/q_client.h \
	common/ref_params.h \
	common/weaponinfo.h \
	common/wrect.h \
	common/xash3d_types.h \
	engine/custom.h \
	engine/keydefs.h \
	engine/menu_int.h \
	engine/mobility_int.h \
	engine/cursor_type.h \
	pm_shared/pm_info.h \
	public/xash3d_mathlib.h \
"

die()
{
	echo $@
	exit 1
}

for i in $FILES_LIST; do
	dir=$(dirname $i)

	echo -n "Copying $i... "
	mkdir -p $dir || die "Can't create folder $dir"
	cp ../../../$i $dir/ || die "Can't copy file $1"
	echo "OK"
done

mkdir -p public
cp ../../library_suffix/include/build.h public/ || die "Can't copy build.h"
cp ../../library_suffix/include/buildenums.h public/ || die "Can't copy buildenums.h"
