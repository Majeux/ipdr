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
$argadd src/algo/ipdr-pebbling.cpp
set stal=2
tabnew
tabnew
tabrewind
edit src/testing/z3-pebbling-model.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
2wincmd h
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
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 96 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 145 + 181) / 363)
argglobal
balt src/model/pdr/pdr-model.cpp
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
let s:l = 134 - ((38 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 134
normal! 034|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
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
let s:l = 43 - ((0 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 43
normal! 03|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/frames.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
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
let s:l = 38 - ((37 * winheight(0) + 40) / 80)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 38
normal! 049|
lcd ~/Documents/master/pebbling-pdr
wincmd w
3wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 96 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 145 + 181) / 363)
tabnext
edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
argglobal
balt ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
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
let s:l = 1 - ((0 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
tabnext
edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
argglobal
balt ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
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
let s:l = 1 - ((0 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
tabnext 1
set stal=1
badd +36 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +62 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +38 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
badd +47 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +27 ~/Documents/master/pebbling-pdr/.clang-format
badd +0 ~/Documents/master/pebbling-pdr/CMakeLists.txt
badd +123 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +91 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +1 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
badd +175 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +58 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/testing/z3-experiments.cpp
badd +28 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +78 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +63 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +40 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-model.h
badd +134 ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
badd +82 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +37 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +43 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +26 ~/Documents/master/pebbling-pdr/src/algo/pdr-alt.cpp
badd +54 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +46 ~/Documents/master/pebbling-pdr/inc/testing/z3pdr.h
badd +194 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +21 ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h
badd +22 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +57 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +28 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +22 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
badd +19 ~/Documents/master/pebbling-pdr/inc/auxiliary/types-ext.h
badd +249 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
badd +61 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/peterson-experiments.h
badd +61 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +191 ~/Documents/master/pebbling-pdr/inc/testing/logger.h
badd +1 ~/Documents/master/pebbling-pdr/;
badd +38 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0 && getbufvar(s:wipebuf, '&buftype') isnot# 'terminal'
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToOFAcI
let s:sx = expand("<sfile>:p:r")."x.vim"
if filereadable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &g:so = s:so_save | let &g:siso = s:siso_save
set hlsearch
nohlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
