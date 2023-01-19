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
edit src/testing/z3ipdr.cpp
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
exe '2resize ' . ((&lines * 55 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 362)
exe '3resize ' . ((&lines * 23 + 41) / 83)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 120 + 181) / 362)
argglobal
balt src/algo/ipdr-pebbling.cpp
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
let s:l = 42 - ((41 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 42
normal! 034|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
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
let s:l = 42 - ((41 * winheight(0) + 27) / 55)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 42
normal! 03|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/.clang-format") | buffer ~/Documents/master/pebbling-pdr/.clang-format | else | edit ~/Documents/master/pebbling-pdr/.clang-format | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/.clang-format
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
let s:l = 28 - ((9 * winheight(0) + 11) / 23)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 28
normal! 042|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/result.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/result.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/result.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/result.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/algo/result.h
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
let s:l = 137 - ((39 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 137
normal! 020|
lcd ~/Documents/master/pebbling-pdr
wincmd w
3wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 362)
exe '2resize ' . ((&lines * 55 + 41) / 83)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 362)
exe '3resize ' . ((&lines * 23 + 41) / 83)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 120 + 181) / 362)
tabnext
edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
argglobal
1argu
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
badd +10 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +42 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/testing/z3-experiments.cpp
badd +28 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +45 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +57 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +13 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-model.h
badd +24 ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
badd +111 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +37 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +175 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +43 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +26 ~/Documents/master/pebbling-pdr/src/algo/pdr-alt.cpp
badd +54 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +62 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +26 ~/Documents/master/pebbling-pdr/inc/testing/z3pdr.h
badd +178 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +21 ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h
badd +22 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +57 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +28 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +22 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
badd +19 ~/Documents/master/pebbling-pdr/inc/auxiliary/types-ext.h
badd +91 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +249 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
badd +61 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/peterson-experiments.h
badd +61 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +191 ~/Documents/master/pebbling-pdr/inc/testing/logger.h
badd +47 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +37 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
badd +137 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +1 ~/Documents/master/pebbling-pdr/;
badd +28 ~/Documents/master/pebbling-pdr/.clang-format
badd +16 ~/Documents/master/pebbling-pdr/CMakeLists.txt
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
