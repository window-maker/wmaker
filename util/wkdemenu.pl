#!/usr/bin/perl
#
#
# kde2wmaker.pl:
#
#
# This script, made for users of Window Maker (http://windowmaker.org) is to
# be used along with KDE (http://www.kde.org).
#
#
# The default directory, ~/.kde/share/applnk/apps, will contain various
# sub-directories such as Development, Editors, Internet, etc. If for some
# reason, you wish to use an alternate (parent) directory that contains the
# various AppName.kdelnk files, it can be specified on the command line.
#
# The directory, if an alternate is specified, MUST be a parent directory to
# any/all sub-directories.
#
# Command line usage:
#	-d <KDE App.kdelnk dir> -f <output menufile> -s yes (print to STDOUT)
#
# Example command with args:
#	-d ~/.kde/share/applnk -f ~/.kde2wmaker.menu -s yes
#
# When the script is run, it will write out a proper Window Maker "External
# Menu" entry, that can be included in the menu. When the External Menu has
# been correctly configured, the root menu will display a sub-menu containing
# all of the KDE related items found. The script only needs to be run when/if
# KDE is updated.
#
# 
# Installation and Configuration:
#
# 1) If /usr/bin/perl is not the location of the perl binary on your system,
#    the first line should be changed to reflect upon it's location.
# 2) Run the script.
# 3) Configure Window Maker's menu by editing ~/GNUstep/Defaults/WMRootMenu
#    This could be done with any text editor, or by using WPrefs. Insert
#    the following line (if done with a text editor) into the WMRootMenu file.
#      ("External Menu", OPEN_MENU, "$HOME/.kde2wmaker.menu"),
#    If done using WPrefs, simply "Add External Menu" from the drop down menu,
#    then type: $HOME/.kde2wmaker.menu   into the "Menu Path/Directory List"
#    textbox.
# 4) Some KDE entries, such as "Pine" will require a terminal to execute it.
#    There is a terminal varable below. You may use any terminal, XTerm is the
#    default. Any command line options such as: -fg -bg, etc. can be
#    specified in this variable as well.
#
#
# Michael Hokenson - logan@dct.com


###
### Variables
###

### The External Menu file, this should NEVER point to the root menu file
$menufile = "$ENV{'HOME'}/.kde2wmaker.menu";

### Base directory, location of all the KDE AppName.kdelnk files
$basedir = "$ENV{'HOME'}/.kde/share/applnk/apps";

### Terminal to use
$term = "xterm";

### Print to STDOUT, default is YES, a filename is specified
$stdout = 1;


###
### Begin work
###

### Process command line arguments
foreach $arg(@ARGV) {
	if($last) {
		if($last eq "-d") {
			$basedir = $arg;
		} elsif($last eq "-f") {
			$menufile = $arg;
			$stdout = 0;
		} 
		undef($last);
	} elsif($arg =~ /^-/) {
		if($arg =~ /^-[dfs]$/) {
			$last = $arg;
		} else {
			die("Unknown option: $arg\n\nUsage: kde2wmaker.pl\n\t-d <KDE App.kdelnk dir> [-f <output menufile>]\n");
			&Usage;
		}
	}
}

### Make sure we actually exist
if(-d $basedir) {

	### Start some error checking
	$errors = 0;

	### See if there is an old menu file. If there is, rename it
	unless($stdout) {
		if(-e $menufile) {
			print "\tFound $menufile, renaming\n\n";
			rename $menufile, "$menufile.old";
		}
	}

	### Read in the directories
	opendir(KDE,$basedir);
	@dirs = readdir(KDE);
	closedir(KDE);

	### Make sure there is actually something in $basedir
	if($#dirs <= 1) {
		print "ERROR:\n\tNothing found in $basedir\n\n";
		exit(0);
	}

	### Begin writing the menu
	unless($stdout) {
		open(MENUFILE,"> $menufile");
	}

	### Start the main menu entry
	if($stdout) {
		print "\t\"KDE Applications\" MENU\n";
	} else {
		print MENUFILE "\t\"KDE Applications\" MENU\n";
	}

	### Begin processing the directories
	foreach $dir(@dirs) {

		### Handle each directory unless if its hidden (starts with .)
		unless($dir =~ /^\./) {
			### Print out the sub directories
			if($stdout) {
				print "\t\t\"$dir\" MENU\n";
			} else {
				print MENUFILE "\t\t\"$dir\" MENU\n";
			}

			### Look in each directory and process individual files
			opendir(SUB,"$basedir/$dir");
			@subdirs = readdir(SUB);
			closedir(SUB);

			### Process files in each sub directory
			foreach $sub(@subdirs) {

				### Once again, process all files but those that are hidden
				unless($sub =~ /^\./) {

					### Open the files
					open(SUB,"$basedir/$dir/$sub");

					### Search through the contents of the file
					while($line = <SUB>) {
						chop($line);
						### Grab the name
						if($line =~ /^Name=/) {
							$pname = $line;
							$pname =~ s/Name=//;
						}
						### Grab the command
						if($line =~ /^Exec=/) {
							$pargs = $line;
							$pargs =~ s/Exec=//;
						}
						### If Terminal=1, then we need to execute a term
						if($line =~ /^Terminal=1$/) {
							$pargs = "$term -T \"$pname\" -e $pargs";
						}
					}

					close(SUB);

					### Some error checking on the Name and Exec
					if($pname eq "") {
						$pname = $sub;
						$pname =~ s/\.kdelnk//;
					}
					if($pargs eq "") {
						$error = 1;
						$pargs = $sub;
						$pargs =~ s/\.kdelnk//;
						print "WARNING:\n\tNo Exec for $pname, using $pargs\n";
					}

					### Begin printing menu items
					if($stdout) {
						print "\t\t\t\"$pname\" EXEC $pargs\n";
					} else {
						print MENUFILE "\t\t\t\"$pname\" EXEC $pargs\n";
					}
				}
			}

			### Print the end of the sub menu
			if($stdout) {
				print "\t\t\"$dir\" END\n";
			} else {
				print MENUFILE "\t\t\"$dir\" END\n";
			}
		}
	}

	### Finish off the main menu entry
	if($stdout) {
		print "\t\"KDE Applications\" END\n";
	} else {
		print MENUFILE "\t\"KDE Applications\" END\n";
	}

	unless($stdout) {
		close(MENUFILE);
	}

	### Yaya!
	if($errors) {
		print "\n.. Finished. There were errors.\n";
	}
# else {
#		print "\n.. Finished.\n";
#	}

	exit(0);
} else {
	### Error out :/
	print "ERROR:\n\t$basedir not found\n\tTry another directory.\n";
	exit(0);
}

###
### End work :))
###
