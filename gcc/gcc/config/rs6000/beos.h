/* Definitions of target machine for GNU compiler,
   for IBM RS/6000 POWER running AIX.
   Copyright (C) 2000-2015 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */


#undef TARGET_OS_CPP_BUILTINS
/* __POWERPC__ must be defined for some header files */
#define TARGET_OS_CPP_BUILTINS()          \
  do                                      \
    {                                     \
      builtin_define ("__ppc__");               \
      builtin_define ("__PPC__");               \
      builtin_define ("__POWERPC__");           \
      builtin_define ("__NATURAL_ALIGNMENT__"); \
      builtin_assert ("system=macos");     \
      builtin_assert ("cpu=powerpc");     \
      builtin_assert ("machine=powerpc"); \
      builtin_define("pascal=__attribute__((__pascal__))");	\
      builtin_define("__IEEE_BIG_ENDIAN"); \
    }                                     \
  while (0)

/* --no-check-sections  :   sections overlap on purpose!
*/

#define LINK_SPEC "%{!r:-btextro} -bhalt:4 -bnodelcsect \
--no-check-sections \
%{shared:-bM:SRE}"

/* TODO: add libroot to some of these, maybe replace -lc */
#define LIB_SPEC "-lc"
#define LIBGCC_SPEC "-lgcc -lretrocrt -lroot -lgcc"
#define LINK_GCC_C_SEQUENCE_SPEC "--start-group -lgcc -lc -lretrocrt -lroot --end-group"

/* TODO: we'll need a startfile... */
#undef STARTFILE_SPEC
#define STARTFILE_SPEC ""

/* GCC >= 14 no longer has XCOFF/stabs debug output, so debug the same way
   as the classic Mac OS PowerPC target (macos.h): DWARF 2 in XCOFF sections.
   TODO: decide what the best debugging story for BeOS will be.  */
#define DWARF2_DEBUGGING_INFO 1
#define PREFERRED_DEBUGGING_TYPE DWARF2_DEBUG
/* gas only knows the XCOFF .dwsect codes for DWARF <= 4; GCC's DWARF 5
   default would emit .debug_rnglists / .debug_loclists / .debug_line_str by
   name, which the assembler rejects.  */
#define DWARF_VERSION_DEFAULT 4
#define DEBUG_INFO_SECTION	"0x10000"
#define DEBUG_LINE_SECTION	"0x20000"
#define DEBUG_PUBNAMES_SECTION	"0x30000"
#define DEBUG_PUBTYPES_SECTION	"0x40000"
#define DEBUG_ARANGES_SECTION	"0x50000"
#define DEBUG_ABBREV_SECTION	"0x60000"
#define DEBUG_STR_SECTION	"0x70000"
#define DEBUG_RANGES_SECTION	"0x80000"
#define DEBUG_LOC_SECTION	"0x90000"
#define DEBUG_FRAME_SECTION	"0xA0000"
#define DEBUG_MACINFO_SECTION	"0xB0000"
#define DEBUG_MACRO_SECTION	"0xB0000"
