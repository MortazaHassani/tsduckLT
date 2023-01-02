This directory contains C++, Java and Python sample coed to use the tsp memory plugins.

Prerequisites:

To be able to build this sample application, you must install the TSDuck
development environment first. On Windows systems, you must select the
optional "Development" component during the installation. On Ubuntu systems,
you must install the package tsduck-dev. On Fedora, Red Hat and CentOS, you
must install the package tsduck-devel.

Building the applications on Linux:

Just run "make". The makefile in this directory contains various calls to
the 'tsconfig' utility which generates the various required compilation
and link options.

Run "make test" to demonstrate the application in the three languages.

Building the C++ application on Windows:

Open the solution file "sample-memory-plugins.sln" using Visual Studio 2017 and build
the application. The project file "sample-memory-plugins.vcxproj" contains the
following additional line, just before the final line:

  <Import Project="$(TSDUCK)\tsduck.props" />

The included property file sets all required options to use the TSDuck library.
