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
edit src/cli-parse.cpp
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
exe 'vert 1resize ' . ((&columns * 136 + 181) / 362)
exe '2resize ' . ((&lines * 37 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 103 + 181) / 362)
exe '3resize ' . ((&lines * 42 + 41) / 83)
exe 'vert 3resize ' . ((&columns * 103 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 121 + 181) / 362)
argglobal
balt inc/model/pdr/pdr-context.h
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
let s:l = 458 - ((38 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 458
normal! 031|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/result.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/result.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/result.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/result.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/testing/logger.h
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
let s:l = 357 - ((33 * winheight(0) + 18) / 37)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 357
normal! 018|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/cli-parse.h
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
let s:l = 16 - ((9 * winheight(0) + 21) / 42)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 16
normal! 0
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
let s:l = 201 - ((62 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 201
normal! 047|
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe 'vert 1resize ' . ((&columns * 136 + 181) / 362)
exe '2resize ' . ((&lines * 37 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 103 + 181) / 362)
exe '3resize ' . ((&lines * 42 + 41) / 83)
exe 'vert 3resize ' . ((&columns * 103 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 121 + 181) / 362)
tabnext 1
badd +55 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +458 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +177 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +56 ~/Documents/master/pebbling-pdr/inc/testing/logger.h
badd +300 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +69 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +110 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +202 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +82 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +112 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +14 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +113 ~/Documents/master/pebbling-pdr/CMakeLists.txt
badd +34 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +10 ~/Documents/master/pebbling-pdr/inc/testing/_logging.h
badd +55 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +53 ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp
badd +137 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +52 ~/Documents/master/pebbling-pdr/inc/auxiliary/types-ext.h
badd +158 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +69 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +93 ~/Documents/master/pebbling-pdr/inc/testing/experiments.h
badd +66 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +1556 ~/vcpkg/installed/x64-linux/include/cxxopts.hpp
badd +7 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +16 ~/Documents/master/pebbling-pdr/inc/algo/result.h
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
