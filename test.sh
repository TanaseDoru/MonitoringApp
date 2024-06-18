#!/bin/bash

choices="$(dialog --stdout --checklist "Select items:" 0 0 0 \
  1 "Choice number one" off \
  2 "Choice number two" on \
  3 "Choice number three" off \
  4 "Choice number four" on)"
(clear)  
echo $choices
#(clear)
