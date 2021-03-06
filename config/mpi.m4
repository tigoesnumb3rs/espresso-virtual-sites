dnl -*- mode: autoconf -*-
dnl Copyright (C) 2010 The ESPResSo project
dnl Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010 Max-Planck-Institute for Polymer Research, Theory Group, PO Box 3148, 55021 Mainz, Germany
dnl  
dnl This file is part of ESPResSo.
dnl  
dnl ESPResSo is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl  
dnl ESPResSo is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl  
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>. 
AC_DEFUN([ES_CHECK_MPI],[
	AC_BEFORE([$0],[ES_CHECK_COMPILER])

	AC_ARG_WITH(mpi,
		AC_HELP_STRING([--with-mpi=TYPE],
			[compile with MPI (parallelization)
			support. If you specify
			TYPE=fake,generic,lam,mpich,poe or dmpi, the
			corresponding MPI implementation will used,
			otherwise the native environment of the
			platform is used. If none is found, the fake
			implementation for only one processor is
			used.]
			),
		,with_mpi=yes)

	AC_ARG_WITH(mpicommand,
		AC_HELP_STRING([--with-mpicommand=COMMAND],
			[choose the command to execute parallel jobs
			(that is, mpirun, poe, dmpirun or similar.
			It has to fit to your MPI environment
			chosen by --with-mpi. If not specified,
			configure will try to guess the location and
			name.]
			),
		,with_mpicommand=)

	# "no" is equivalent to "fake", "yes" means to guess
	if test x$with_mpi = xfake; then
		ES_MPI_SETUP_FAKE
	elif test x$with_mpi = xno; then
		with_mpi=fake
		ES_MPI_SETUP_FAKE
	else
		if test x$with_mpi = xyes; then
			ES_MPI_GUESS_ENV
		else
			ES_MPI_SETUP($with_mpi)
		fi
	fi
	AC_DEFINE_UNQUOTED(MPI,"$with_mpi",[Which MPI implementation to use?])
	AM_CONDITIONAL(MPI_FAKE, [test x$with_mpi = xfake])
	AC_SUBST(MPI_INVOCATION)
])

AC_DEFUN([ES_MPI_SETUP],[
	if test x$1 = x; then ES_MPI_GUESS_ENV
	else
		case $1 in
		poe)	ES_MPI_FIND_MPICC
			ES_MPI_FIND_POE
			if test $poe_found != yes; then AC_MSG_ERROR([IBM POE MPI not working]); fi
			;;
		dmpi)	ES_MPI_FIND_DMPI
			if test $dmpi_found != yes; then AC_MSG_ERROR([Tru64/OSF MPI not working]); fi
			;;
		generic) ES_MPI_FIND_MPICC
			ES_MPI_FIND_GENERIC
			if test $generic_found != yes; then AC_MSG_ERROR([generic MPI not working]); fi
			;;
		lam)	ES_MPI_FIND_MPICC
			ES_MPI_FIND_LAM
			if test $lam_found != yes; then	AC_MSG_ERROR([LAM/MPI not working]); fi
			;;
		mpich)	ES_MPI_FIND_MPICH
			if test $mpich_found != yes; then AC_MSG_ERROR([MPICH not working]); fi
			;;
		esac
	fi
])

AC_DEFUN([ES_MPI_GUESS_ENV],[
	with_mpi=guess
	dnl look for special implementations
	case $target_os in
	*aix*)	ES_MPI_FIND_MPICC
		mpicc_done=y
		ES_MPI_FIND_POE
		if test .$poe_found = .yes; then with_mpi=poe; fi
		;;
	*osf*)	ES_MPI_FIND_DMPI
		if test .$dmpi_found = .yes; then with_mpi=dmpi; fi
		;;
	esac

	dnl always check for generic, LAM/MPI and MPICH, as they are multiplatform

	if test x$with_mpi = xguess; then
		if test x$mpicc_done = x; then
			ES_MPI_FIND_MPICC
		fi
		ES_MPI_FIND_LAM
		if test x$lam_found = xyes; then with_mpi=lam; fi
	fi

	if test x$with_mpi = xguess; then
		ES_MPI_FIND_MPICH
		if test .$mpich_found = .yes; then with_mpi=mpich; fi
	fi

	dnl generic last, since here we just assume how to run mpirun

	if test x$with_mpi = xguess; then
		if test x$mpicc_done = x; then
			ES_MPI_FIND_MPICC
		fi
		ES_MPI_FIND_GENERIC
		if test x$generic_found = xyes; then with_mpi=generic; fi
	fi

	dnl out of guesses
	if test .$with_mpi = .guess; then
		issue_fake_warning=yes
		with_mpi=fake
		ES_MPI_SETUP_FAKE
	fi
])

AC_DEFUN([ES_MPI_SETUP_FAKE],[
	if test "$with_mpicommand" != ""; then
	   AC_MSG_ERROR([--with-mpicommand cannot be specified.
	   Espresso is configured to use the fake MPI environment, which does not
	   need a command to run mpi programs. Either you specify your MPI
	   environment using --with-mpi, or you do not specify --with-mpicommand.
	   In this case, the fake MPI-Implementation for a single processor is
	   used])
	fi
	MPI_INVOCATION="\$ESPRESSO_CALL"
])

AC_DEFUN([ES_MPI_FIND_MPICC],[
	# only test the compiler if not overridden by the user
	if test .$ac_env_CC_set != .set; then
		AC_CHECK_PROGS(MPICC,[mpcc mpxlc mpicc mpicci mpiccg hcc cc icc gcc])
		CC=$MPICC
	fi
	AC_MSG_CHECKING([whether the $CC command works out of the box for MPI])
	AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY(MPI_Init)],
    		[AC_MSG_RESULT(yes); mpicc_works=yes],[AC_MSG_RESULT(no); mpicc_works=no])
])

AC_DEFUN([ES_MPI_FIND_GENERIC],[
	AC_MSG_NOTICE([trying to find a generic MPI])
	generic_found=yes
	if test x$mpicc_works != xyes; then
		generic_found=no
	else
		if test x$with_mpicommand != x; then
		   generic_bin=$with_mpicommand
		else
		   AC_PATH_PROG(generic_bin, mpirun, mpirun, [$PATH])
		   if test x$generic_bin = x; then
			generic_found=no
		   fi
		fi
	fi
	if test x$generic_found = xyes; then
		MPI_INVOCATION="$generic_bin -np \$NP \$ESPRESSO_CALL"
		AC_MSG_NOTICE([found a generic MPI environment])
	fi
])

AC_DEFUN([ES_MPI_FIND_LAM],[
	AC_MSG_NOTICE([trying to find a LAM environment])
	lam_found=yes
	if test $mpicc_works = yes; then
		lam_cppflags=$CPPFLAGS
		lam_ldflags=$LDFLAGS
		lam_libs=$LIBS
	else
		save_cppflags=$CPPFLAGS
		save_ldflags=$LDFLAGS
		save_libs=$LIBS

		AC_MSG_CHECKING([whether the $CC command works without additional flags])
		lam_cppflags=$CPPFLAGS
		lam_ldflags=$LDFLAGS
		lam_libs="$LIBS -lmpi -llam -lpthread"
		LIBS=$lam_libs
		AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY(MPI_Init)],
    			[AC_MSG_RESULT(yes); mpicc_works_wl=yes],[AC_MSG_RESULT(no); mpicc_works_wl=no])
		if test .$mpicc_works_wl = .no; then
			dnl guess the library paths or the include paths.
			for hdrprf in /opt/lam /usr/lam /usr/local/lam /usr /usr/local; do
				if test -f $hdrprf/include/lam.h; then lam_hdr_prf=$hdrprf; lam_hdr=$hdrprf/include; break; fi
			done
			if test $lam_hdr. = .; then
				AC_MSG_NOTICE([did not find lam.h, if you want to use LAM, please specify its location via CPPFLAGS])
				lam_found=no
			fi
			for libprf in /opt/lam /usr/lam /usr/local/lam /usr /usr/local; do
				for lib in $libprf/lib64 $libprf/lib; do
					if test -f $lib/liblam.a; then lam_lib_prf=$libprf; lam_lib=$lib; break; fi
				done
				if test .$lam_lib != .; then break; fi
			done
			if test $lam_lib. = .; then
				AC_MSG_NOTICE([did not find lam library, if you want to use LAM, please specify its location and name via LDFLAGS])
				lam_found=no
			fi
			if test $lam_found = yes; then
				lam_cppflags="$CPPFLAGS -I$lam_hdr"
				lam_ldflags="$LDFLAGS -L$lam_lib"
				CPPFLAGS=$lam_cppflags
				LDFLAGS=$lam_ldflags
				AC_MSG_CHECKING([whether the $CC command works with guessed headers and libraries])
	 			AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY(MPI_Init)],
		    			AC_MSG_RESULT(yes),[AC_MSG_RESULT(no); lam_found=no])
			fi
		fi
		CPPFLAGS=$save_cppflags
		LDFLAGS=$save_ldflags
		LIBS=$save_libs
	fi
	dnl last but not least the invocation, checking for mpirun
	if test "$with_mpicommand" != ""; then
	   lam_bin=$with_mpicommand
	else
	   AC_PATH_PROG(lam_bin, mpirun, mpirun, [$PATH:$lam_lib_prf/bin:$lam_hdr_prf/bin])
	fi
	dnl check the program that it actually belongs to LAM
	$lam_bin -h > conftmp.log 2>&1
	if ! grep LAM conftmp.log >/dev/null 2>&1; then
		AC_MSG_NOTICE([mpirun binary does not belong to LAM or was not found,
  if you want to use LAM, please add specify the location of mpirun using --with-mpicommand])
		lam_found=no
	fi
	rm -f conftmp.log
	if test $lam_found = yes; then
		CPPFLAGS=$lam_cppflags
		LDFLAGS=$lam_ldflags
		LIBS=$lam_libs
		MPI_INVOCATION="$lam_bin -x ESPRESSO_SCRIPTS -np \$NP -nsigs \$ESPRESSO_CALL"
		AC_MSG_NOTICE([found a working LAM environment])
	fi
])

AC_DEFUN([ES_MPI_FIND_MPICH],[
	AC_MSG_NOTICE([trying to find a MPICH environment])
	mpich_found=yes
	save_cppflags=$CPPFLAGS
	save_ldflags=$LDFLAGS
	save_libs=$LIBS

	AC_MSG_CHECKING([whether the $CC command works without flags])
	mpich_cppflags=$CPPFLAGS
	mpich_ldflags=$LDFLAGS
	mpich_libs="$LIBS -lmpich"
	LIBS=$mpich_libs
	AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY(MPI_Init)],
    		[AC_MSG_RESULT(yes); cc_works_wl=yes],[AC_MSG_RESULT(no); cc_works_wl=no])
	if test .$cc_works_wl = .no; then
		dnl guess the library paths or the include paths.
		for hdrpprf in /opt/mpich /usr/mpich /usr/local/mpich /usr /usr/local; do
			dnl test directly ...
			if test -f $hdrpprf/include/mpi.h; then mpich_hdr_prf=$hdrpprf; mpich_hdr=$hdrpprf/include; break; fi
			dnl ... but it may as well be a subdir
			for hdrprf in $hdrpprf/*; do
				if test -d $hdrprf; then
					if test -f $hdrprf/include/mpi.h; then mpich_hdr_prf=$hdrprf; mpich_hdr=$hdrprf/include; break; fi
				fi
				if test .$mpich_hdr != .; then break; fi
			done
			if test .$mpich_hdr != .; then break; fi
		done
		if test $mpich_hdr. = .; then
			AC_MSG_NOTICE([did not find mpi.h, please specify its location via CPPFLAGS])
			mpich_found=no
		fi
		for libpprf in /opt/mpich /usr/mpich /usr/local/mpich /usr /usr/local; do
			dnl test directly ...
			for lib in $libpprf/lib64 $libpprf/lib; do
				if test -f $lib/libmpich.a; then mpich_lib_prf=$libpprf; mpich_lib=$lib; break; fi
			done
			dnl ... but it may as well be a subdir
			for libprf in $libpprf/*; do
				if test -d $libprf; then
					for lib in $libprf/lib64 $libprf/lib; do
						if test -f $lib/libmpich.a; then mpich_lib_prf=$libprf; mpich_lib=$lib; break; fi
					done
				fi
				if test .$mpich_lib != .; then break; fi
			done
			if test .$mpich_lib != .; then break; fi			
		done
		if test $mpich_lib. = .; then
			AC_MSG_NOTICE([did not find mpich library, please specify its location and name via LDFLAGS])
			mpich_found=no
		fi
		if test $mpich_found = yes; then
			mpich_cppflags="$CPPFLAGS -I$mpich_hdr"
			mpich_ldflags="$LDFLAGS -L$mpich_lib"
			CPPFLAGS=$mpich_cppflags
			LDFLAGS=$mpich_ldflags
			AC_MSG_CHECKING([whether the $CC command works with guessed headers and libraries])
	 		AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY(MPI_Init)],
		    		AC_MSG_RESULT(yes),[AC_MSG_RESULT(no); mpich_found=no])
			CPPFLAGS=$save_cppflags
			LDFLAGS=$save_ldflags
		fi
	fi
	CPPFLAGS=$save_cppflags
	LDFLAGS=$save_ldflags
	LIBS=$save_libs

	dnl last but not least the invocation, checking for mpirun
	if test "$with_mpicommand" != ""; then
	   mpich_bin=$with_mpicommand
	else
	   AC_PATH_PROG(mpich_bin, mpirun, mpirun, [$PATH:$mpich_lib_prf/bin:$mpich_hdr_prf/bin])
	fi

	dnl check the program that it actually belongs to MPICH
	$mpich_bin -h > conftmp.log 2>&1
	if ! grep mpich conftmp.log >/dev/null 2>&1; then
		AC_MSG_NOTICE([mpirun binary does not belong to MPICH or was not found,
  if you want to use MPICH, please add specify the location of mpirun using --with-mpicommand])
		mpich_found=no
	fi
	rm -f conftmp.log
	if test $mpich_found = yes; then
		CPPFLAGS="$mpich_cppflags"
		LDFLAGS=$mpich_ldflags
		LIBS=$mpich_libs
		MPI_INVOCATION="$mpich_bin -np \$NP \$ESPRESSO_CALL"
		AC_MSG_NOTICE([found a working MPICH environment])
	fi
])

AC_DEFUN([ES_MPI_FIND_POE],[
	poe_found=yes
	if test .$mpicc_works != .yes; then
		AC_MSG_NOTICE([the poe environment only works via mpcc, please specify your c compiler])
		poe_found=no
	fi
	if test .$poe_found = .yes; then
		if test "$with_mpicommand" != ""; then
		   poe_bin=$with_mpicommand
		else
		   AC_PATH_PROG(poe_bin, poe, poe, [$PATH])
		fi
		MPI_INVOCATION="$poe_bin \$ESPRESSO_CALL -procs \$NP"
	fi
])

AC_DEFUN([ES_MPI_FIND_DMPI],[
	saved_libs=$LIBS
	dmpi_found=yes
	AC_MSG_CHECKING([whether the $CC command works for MPI])
	dmpi_libs="$LIBS -lmpi"
	LIBS=$dmpi_libs
	AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY(MPI_Init)],
    			[AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no); dmpi_found=no])
	if test .$dmpi_found = .yes; then
		if test "$with_mpicommand" != ""; then
		   dmpi_bin=$with_mpicommand
		else
		   AC_PATH_PROG(dmpi_bin, dmpirun, dmpirun, [$PATH])
		fi
		MPI_INVOCATION="$dmpi_bin -np \$NP \$ESPRESSO_CALL"
	fi
])
