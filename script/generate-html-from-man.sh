#!/bin/sh
###########################################################################
#
#  Window Maker window manager
#
#  Copyright (c) 2021 Christophe CURIS
#  Copyright (c) 2021 Window Maker Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program. If not, see <http://www.gnu.org/licenses/>.
#
###########################################################################
#
# generate-html-from-man.sh:
#   Convert a man page into HTML using Groff, then post-process it to make
#   it fit the Window Maker site's theme and install it into the website
#   Git repository.
#
# The pages are generated as xhtml because that's the most convenient
# format at current time that Groff can provide. The post-processing of the
# page includes:
#
# - the basic style sheet included by groff is replaced by a call to "manpage.css";
#
# - if a menu list is provided with "--with-menu", then it is added at the
# beginning of the <body> inside an "ul" with class="menu";
#
# - the list of sections at the beginning is enclosed in a div with class="toc";
#
# - emails addresses in the form "<name@example.com>" are obfuscated (to
# not help spambots) and a mailto: link is added;
#
# - a link is added when an url of the form "http://..." is encountered;
#
# - when a man-page reference is found, in the form "name(section)", a link
# is added, either to a local page if in the "--local-pages" list or to the
# external url provided in "--external-url";
#
# - if the package name and version is provided with "--package", then a div with
# class="footer" is added at the end of the <body>.
#
###########################################################################
#
# For portability, we stick to the same sh+awk constraint as Autotools to
# limit problems, see for example:
#   http://www.gnu.org/software/autoconf/manual/autoconf-2.69/html_node/Portable-Shell.html
#
###########################################################################

# Report an error on stderr and exit with status 2 to tell make that we could
# not do what we were asked
arg_error() {
    echo "`basename $0`: $@" >&2
    exit 2
}

# print help and exit with success status
print_help() {
    echo "$0: convert man page into HTML"
    echo "Usage: $0 options... man-page-file"
    echo "valid options are:"
    echo "  --external-url tmpl : use tmpl as URL template for non-local man pages"
    echo "  --groff name        : name of groff binary to use"
    echo "  --local-pages names : list of man pages that are available locally"
    echo "  --output filename   : name of the HTML file to generate"
    echo "  --with-menu code    : insert the specified code to add a navigation menu"
    echo "  --package name      : name of package, added in the footer of page"
    exit 0
}

# Sets the default values
GROFF=groff

# Extract command line arguments
while [ $# -gt 0 ]; do
    case $1 in
        --external-url)
            shift
            [ "x$external_template" = "x" ] || arg_error "only 1 template for URL can be specified (option: --external-url)"
            external_template=$1
          ;;

        --groff)
            shift
            GROFF=$1
          ;;

        --local-pages)
            shift
            local_pages="$local_pages $1 "
          ;;

        --output)
            shift
            [ -z "$output_file" ] || arg_error "only 1 target file can be specified (option: --output)"
            output_file="$1"
          ;;

        --with-menu)
            shift
            [ -z "$menu_line" ] || arg_error "only 1 code for the menu can be specified (option: --with-menu)"
            menu_line=`echo "$1" | sed -e 's/"/\\\"/g' `
          ;;

        --package)
            shift
            [ -z "$package_line" ] || arg_error "package name can be specified only once (option: --package)"
            package_line="$1"
          ;;

        -h|-help|--help) print_help ;;
        -*) arg_error "unknow option '$1'" ;;

        *)
            [ -z "$input_file" ] || arg_error "only 1 man page can be specified"
            input_file=$1
          ;;
    esac
    shift
done

# Check consistency of command-line
[ -z "$input_file" ] && arg_error "no source man page given"
[ -f "$input_file" ] || arg_error "man page \"$input_file\" cannot be read"

if [ -n "$external_template" ]; then
    echo "$external_template" | grep '%[plu]' > /dev/null \
	|| arg_error "no marker %p, %l or %u for the page name in the URL template (option: --external-url)"
fi

# Generate the name of the output file from the man page file name
if [ -z "$output_file" ]; then
    output_file=`echo "$input_file" | sed 's,^.*/,,g ; s/\.[^.]*$//' `.html
fi

###########################################################################

# Pass the configuration to the AWK script; remove the path information
awk_init="BEGIN {
  external_template = \"$external_template\";
  known_pages = \" $local_pages \";
  menu_code = \"$menu_line\";
  package_line = \"$package_line\";
  source_name = \"`basename "$input_file" `\";
}"

# This AWK script is post-processing the HTML generated by Groff
awk_process_html='
/<html>/ {
  print;
  $0 = "<!-- This file is generated automatically by a script, do not edit -->";
}

# Groff is generating a title with the name of the command, followed by a table of content
# We detect all this and embed it into a <div> section for presentation purpose
/<h1/ {
  header = gensub(/<h1[^>]*>([^<]*)<\/h1>/, "\\1", 1);
  print "  <div class=\"toc\">";
  print "    <label for=\"toc-checkbutton\">" header "</label>";
  print "    <input type=\"checkbox\" id=\"toc-checkbutton\" checked=\"true\">";
  print "    <div id=\"toc-list\">";
  getline;
  while (!/<hr>/) {
    if (length($0) > 0) {
      print "      " $0;
    }
    getline;
  }
  print "      <hr>";
  print "      <a href=\".\" class=\"return\">Return to list</a><br>";
  print "    </div>";
  print "  </div>";
  print "";
  $0 = "<article>";
}

# Groff is adding an horizontal line at the end of the page, we remove it;
# we take the opportunity to close the <article> that we opened after the toc
/<hr>/ {
  $0 = "</article>"
}

# Groff includes a basic CSS style, we want to replace it with our own to be consitent with
# the general website style
/<style/ {
  while (!/<\/style>/) {
    getline;
  }
  print "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">";
  $0 = "<link rel=\"stylesheet\" type=\"text/css\" href=\"/manpage.css\">"
}

# Detect the email address and add a link; take the opportunity to conceal it the same way it is
# done for the mailing list, to make spammers work less easy
/&lt;[^@ ]*@[^<@> ]*&gt;/ && inside_body {
  line = "";
  while (1) {
    idx_start = match($0, /&lt;[^@ ]*@[^<@> ]*&gt;/);
    if (idx_start == 0) break;

    email = substr($0, idx_start + 4, RLENGTH - 8);
    gsub(/@/, " (at) ", email);
    gsub(/\./, " (dot) ", email);

    line = line substr($0, 1, idx_start - 1) "&lt;";
    line = line "<a href=\"mailto:" email "\">" email "</a>&gt;";
    $0 = substr($0, idx_start + RLENGTH);
  }
  $0 = line $0;
}

# Detect urls and add a link
/http:\/\// && inside_body {
  line = "";
  while (1) {
    idx_start = index($0, "http://");
    if (idx_start == 0) break;

    line = line substr($0, 1, idx_start - 1);
    $0 = substr($0, idx_start);
    idx_end = match($0, /[ \t&]/);
    if (idx_end == 0) idx_end = length($0) + 1;

    line = line "<a href=\"" substr($0, 1, idx_end - 1) "\">";
    line = line substr($0, 1, idx_end - 1) "</a>";

    $0 = substr($0, idx_end);
  }
  $0 = line $0;
}

# Detect references to other man pages and add a link
/<b>[A-Za-z_0-9]*<\/b>\([1-9][a-z]*\)/ {
  line = "";
  while (1) {
    idx_start = match($0, /<b>[A-Za-z_0-9]*<\/b>\([1-9][a-z]*\)/);
    if (idx_start == 0) break;

    line = line substr($0, 1, idx_start - 1);
    split(substr($0, idx_start + 3), elements, /<\/b>\(|\)/);
    # elements[1] contains the name of the man-page
    # elements[2] contains the section
    $0 = substr($0, idx_start + RLENGTH);

    idx_known = match(known_pages, " ([^ ]*/)*" elements[1] "\\." elements[2]);
    if (idx_known == 0) {
      if (external_template == "") {
        print "Error: no URL provided for external man page, needed for \"" elements[1] "(" elements[2] ")\" at line " NR > "/dev/stderr";
        exit(2);
      }
      url = gensub(/%s/, elements[2], "g", external_template);
      url = gensub(/%p/, elements[1], "g", url);
      url = gensub(/%l/, tolower(elements[1]), "g", url);
      url = gensub(/%u/, toupper(elements[1]), "g", url);
    } else {
      # When it is a known page, use the data from known_pages to have possible path information
      url = substr(known_pages, idx_known + 1, RLENGTH - length(elements[2]) - 2) ".html";
    }

    line = line "<a href=\"" url "\"><tt>" elements[1] "</tt>(" elements[2] ")</a>";
  }
  $0 = line $0;
}

# Detect the start of the page to insert the navigation menu code
/<body>/ {
  inside_body = 1;
  print;
  print "<div id=\"wrapper\">";
  if (menu_code) {
    while (getline < menu_code) {
      print;
    }
  }
  next;
}
/<\/body>/ {
  if (package_line) {
    print "<div class=\"footer\">"
    print "  This man page is part of <div id=\"pkgname\">" package_line "</div>";
    print "</div>"
  }
  print "  <div id=\"titlebar\">";
  print "    <div id=\"minimize\"></div>";
  print "    <div id=\"titlebar-inner\">Window Maker Manual: " gensub(/\.(.*)$/, "(\\1)", 1, source_name) "</div>";
  print "    <div id=\"close\"></div>";
  print "  </div>";
  print "  <div id=\"resizebar\">";
  print "    <div id=\"resizel\"></div>";
  print "    <div id=\"resizebar-inner\"></div>";
  print "    <div id=\"resizer\"></div>";
  print "  </div>";
  print "</div>"; # close "wrapper"
  inside_body = 0;
}

{ print }
'

###########################################################################
$GROFF -man -Dutf8 -Thtml < "$input_file" | \
  awk "$awk_init$awk_process_html" > "$TARGET/$output_file" \
  || exit $?
