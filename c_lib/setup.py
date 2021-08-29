from distutils.core import setup, Extension

module = Extension("c_lib",
		sources = ["search_utils.cpp"])

setup(name="c_lib", version="1.0", description="c computing functions", ext_modules=[module])