# Borealis
Aurora Borealis StreamProcessing (http://cs.brown.edu/research/borealis/public/)

Before You Begin

Borealis has been built on Debian, Red Hat, Fedora Core 2, and Mandrake GNU/Linux. Although we are unaware of any dependencies that may prevent it from installing and running cleanly on other varients of GNU/Linux, we can make no such assurances. We have performed a partial installation on the Mac Os X Darwin release. The versions for some packages used in this effort are listed in case someone wants to attempt to complete the installation for the Mac. It can not run on a Mac as is.

The installation process is complex. The software depends on external packages, Unix environemts vary from site to site, and people have their own ways of setting up their environments. Consequently we can not provide rote instructions for installing and running Borealis systems. You will need to read about and understand the processes related to each component and determine how to apply them in your environment.

Read through these instructions and develop an installation plan. Determine which optional packages you'll want to install. Locate and read documentation for the external packages. You should have a good idea how you want to lay out the primary directories and check space requirements. You'll also want to determine settings for environment variables you'll be introducing for the build process.

Primary directories are needed for your application development, the Borealis software, the NMSTL library, Borealis utilities, and external package binaries and their build areas. You may want the directory for the Borealis source tree to be your home directory or a separate work area. External packages, their build areas, the NMSTL library, and Borealis utilities can be located in areas common to multiple users or in a private work area.


Installing Borealis

First install the external Packages Needed For Borealis.

Borealis relies heavily on the Networks, Messaging, Servers, and Threading Library (NMSTL), a portable C++ library for writing networking and messaging components. We distribute the NMSTL package along with the Borealis software. Although NMSTL is publically available, the public version is unsupported and has been modified slightly for Borealis. Only the version that is distributed with Borealis should be used.

Choose a directory where you'd like to install Borealis and NMSTL. You may want to put it in your home directory or a separate work area. Unroll the distribution tar file for Borealis and there will be two top-level directories: nmstl/ and borealis/


Building Borealis

The Borealis software and it's subsystems can all be built with the script: borealis/utility/unix/build.borealis.sh

Carefully read the comments at the top of script and setup your environment accordingly. Here are some sample commands for login scripts that may help you set up a .cshrc login or a .bashrc login file.

You can build Borealis only with gcc 3.3.5 and above. There is a spurious warning was issued in gcc versions 3.3.6 through 4.0.4 that was fixed in the 4.1.1 compiler. If you are using an afflicted compiler you will need to use the gcc -w option to suppress warnings. Note that this means you might not see some useful warnings, so you might want to install a newer compiler.

You can use ccache speed up Borealis builds. Install ccache and set the environment variable:

CXX="ccache g++"; export  CXX         #sh shell
CXX="ccache g++ -w"; export  CXX      #sh shell; gcc 3.3.6 to 4.0.4
setenv  CXX  "ccache g++"             #csh shell
setenv  CXX  "ccache  g++  -w"        #csh shell; gcc 3.3.6 to 4.0.4
Running the build script with no arguments builds the core system and nmstl.

> build.borealis.sh
When NMSTL is built it creates an installed copy in: ${HOME}/install_nmstl/

Once NMSTL is built you can use the copy you just built. You can leave it under your home directory or copy it to a common area such as the place where all the other packages were installed. After that you can avoid rebuilding it by passing the location in the INSTALL_NMSTL variable or with the -nmstl command argument.

At a minimum you will also want to build the client code, the Head, and the marshaling tool.

> build.borealis.sh  -client  -tool.marshal  -tool.head
In order to run the tools, directories containing the Head and marshaling tool need to be included in your PATH variable. You can either copy them to a directory already on the PATH variable or add the tool directories borealis/tool/head/ and borealis/tool/marshal/ to the PATH.


Testing Borealis

To see that Borealis runs properly, build and run the automated test suite. It will run a few simple tests to ensure that basic operations are working. Read the comments in the borealis/test/valid/validate.go.sh script to see how to set up reference output for regression testing.

borealis> build.borealis.sh  -test.valid
borealis> test/valid/validate.go.sh  -rebase   # Establish reference output.
validate.go.sh:  Rebasing output to:  /pro/borealis/utility/valid
validate.go.sh:  Running the catalog test ...
   ... The catalog test ran successfully.
validate.go.sh:  Running the system test ...
       Skipping as there is no output to rebase.
   ... The system test ran successfully.
validate.go.sh:  Running the move test ...
       Skipping as there is no output to rebase.
   ... The move test ran successfully.
validate.go.sh:  Running the two_chains test ...
       Skipping as there is no output to rebase.
   ... The two_chains test ran successfully.
validate.go.sh:  Running the global test ...
   ... The global test ran successfully.

validate.go.sh:  ****  All validation tests were rebased successfully.  ****
borealis> test/valid/validate.go.sh            # Run all validation tests.
validate.go.sh:  Running the catalog test ...
   ... The catalog test ran successfully.
validate.go.sh:  Running the system test ...
   ... The system test ran successfully.
validate.go.sh:  Running the move test ...
   ... The move test ran successfully.
validate.go.sh:  Running the two_chains test ...
   ... The two_chains test ran successfully.
validate.go.sh:  Running the global test ...
   ... The global test ran successfully.

validate.go.sh:  ****  All validation tests ran successfully.  ****

Running simple applications

You can write your Borealis applications in either C++ or Java.

All sample applications are located under borealis/test/. You can compile all the sample applications as follows

> build.borealis.sh  -test.simple       # Build tests in:  borealis/test/simple/
Or you can follow the usual manual instructions:


> cd borealis/test/
> ./setup
> wtf configure [--with-nmstl=/my/location] [...]
> make
To see how to run simple C++ Borealis applications see the examples in the borealis/test/simple/ directory. The examples include both single site and distributed applications. A README file explains how to run the examples.

The Stream Viewer is a graphical user interface included in the Borealis code base. Please consult the "Borealis Application Programmer's Guide" for further information about writing Borealis applications.



Support Has Been Discontinued

Borealis is a collaborative research project developed by professors, students, and staff at Brown, MIT, Brandeis, and the University of Washington. The project has been retired and we are no longer able to provide support.

Thanks for your interest in our software!
