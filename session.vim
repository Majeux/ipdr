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
edit src/algo/generalize.cpp
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
exe 'vert 1resize ' . ((&columns * 120 + 181) / 362)
exe '2resize ' . ((&lines * 45 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 119 + 181) / 362)
exe '3resize ' . ((&lines * 34 + 41) / 83)
exe 'vert 3resize ' . ((&columns * 119 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 121 + 181) / 362)
argglobal
balt src/algo/pdr-logging.cpp
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
let s:l = 86 - ((50 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 86
normal! 015|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/cli-parse.cpp") | buffer ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | else | edit ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
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
let s:l = 379 - ((31 * winheight(0) + 22) / 45)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 379
normal! 07|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/solver.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/solver.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/solver.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
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
let s:l = 109 - ((10 * winheight(0) + 17) / 34)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 109
normal! 028|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/cli-parse.cpp") | buffer ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | else | edit ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
endif
balt ~/Documents/master/pebbling-pdr/CMakeLists.txt
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
let s:l = 484 - ((57 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 484
normal! 016|
lcd ~/Documents/master/pebbling-pdr
wincmd w
3wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 362)
exe '2resize ' . ((&lines * 45 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 119 + 181) / 362)
exe '3resize ' . ((&lines * 34 + 41) / 83)
exe 'vert 3resize ' . ((&columns * 119 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 121 + 181) / 362)
tabnext 1
badd +61 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +380 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +45 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +416 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +26 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +10 ~/Documents/master/pebbling-pdr/inc/testing/_logging.h
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/logger.h
badd +113 ~/Documents/master/pebbling-pdr/CMakeLists.txt
badd +153 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +21 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +53 ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp
badd +89 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +52 ~/Documents/master/pebbling-pdr/inc/auxiliary/types-ext.h
badd +158 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +69 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +93 ~/Documents/master/pebbling-pdr/inc/testing/experiments.h
badd +66 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +1556 ~/vcpkg/installed/x64-linux/include/cxxopts.hpp
badd +79 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +7 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +0 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
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
