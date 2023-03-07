let SessionLoad = 1
let s:so_save = &g:so | let s:siso_save = &g:siso | setg so=0 siso=0 | setl so=-1 siso=-1
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd ~/Documents/master/pebbling-pdr
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
set shortmess=aoO
argglobal
%argdel
$argadd src/cli-parse.cpp
edit src/algo/pdr-logging.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
2wincmd h
wincmd w
wincmd _ | wincmd |
split
1wincmd k
wincmd w
wincmd w
let &splitbelow = s:save_splitbelow
let &splitright = s:save_splitright
wincmd t
let s:save_winminheight = &winminheight
let s:save_winminwidth = &winminwidth
set winminheight=0
set winheight=1
set winminwidth=0
set winwidth=1
exe 'vert 1resize ' . ((&columns * 88 + 158) / 317)
exe '2resize ' . ((&lines * 39 + 39) / 79)
exe 'vert 2resize ' . ((&columns * 121 + 158) / 317)
exe '3resize ' . ((&lines * 36 + 39) / 79)
exe 'vert 3resize ' . ((&columns * 121 + 158) / 317)
exe 'vert 4resize ' . ((&columns * 106 + 158) / 317)
argglobal
balt src/algo/generalize.cpp
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 29 - ((17 * winheight(0) + 38) / 76)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 29
normal! 040|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/testing/logger.h") | buffer ~/Documents/master/pebbling-pdr/inc/testing/logger.h | else | edit ~/Documents/master/pebbling-pdr/inc/testing/logger.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/testing/logger.h
endif
balt ~/Documents/master/pebbling-pdr/src/algo/result.cpp
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 58 - ((30 * winheight(0) + 19) / 39)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 58
normal! 019|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/algo/pdr.h") | buffer ~/Documents/master/pebbling-pdr/inc/algo/pdr.h | else | edit ~/Documents/master/pebbling-pdr/inc/algo/pdr.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
endif
balt ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 70 - ((8 * winheight(0) + 18) / 36)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 70
normal! 030|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/stats.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/stats.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/stats.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/testing/stats.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 1 - ((0 * winheight(0) + 38) / 76)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
4wincmd w
exe 'vert 1resize ' . ((&columns * 88 + 158) / 317)
exe '2resize ' . ((&lines * 39 + 39) / 79)
exe 'vert 2resize ' . ((&columns * 121 + 158) / 317)
exe '3resize ' . ((&lines * 36 + 39) / 79)
exe 'vert 3resize ' . ((&columns * 121 + 158) / 317)
exe 'vert 4resize ' . ((&columns * 106 + 158) / 317)
tabnext 1
badd +132 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +377 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +29 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +112 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +110 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +21 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +113 ~/Documents/master/pebbling-pdr/CMakeLists.txt
badd +26 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +10 ~/Documents/master/pebbling-pdr/inc/testing/_logging.h
badd +185 ~/Documents/master/pebbling-pdr/inc/testing/logger.h
badd +153 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +53 ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp
badd +137 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +52 ~/Documents/master/pebbling-pdr/inc/auxiliary/types-ext.h
badd +158 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +69 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +93 ~/Documents/master/pebbling-pdr/inc/testing/experiments.h
badd +66 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +1556 ~/vcpkg/installed/x64-linux/include/cxxopts.hpp
badd +53 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +7 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +50 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +0 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +16 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +300 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0 && getbufvar(s:wipebuf, '&buftype') isnot# 'terminal'
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToOFAcI
let &winminheight = s:save_winminheight
let &winminwidth = s:save_winminwidth
let s:sx = expand("<sfile>:p:r")."x.vim"
if filereadable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &g:so = s:so_save | let &g:siso = s:siso_save
set hlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
