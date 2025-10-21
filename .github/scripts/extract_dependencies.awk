/Requirements:/ { in_section=1; next }
in_section && /^- / { print substr($0, 3); last_dep=NR }
NR > last_dep && in_section { exit }
