c++ \
	-std=c++20 \
	-o headgasket \
	-g \
	-DV8_ENABLE_WEBASSEMBLY \
	-DV8_ENABLE_SANDBOX \
	-DV8_COMPRESS_POINTERS \
	-DV8_EXTERNAL_CODE_SPACE \
	-DV8_INTL_SUPPORT \
	-I. \
	-Iv8 \
	-Iv8/include \
	-Iv8/out.gn/x64.release.sample/gen/ \
	-fno-rtti \
	main.cpp \
	-Lv8/out.gn/x64.release.sample/obj/ \
	-lv8_monolith \
	-lv8_libbase \
	-lv8_libplatform \
	-ldl \
	-pthread \