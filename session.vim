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
$argadd src/testing/stats.cpp
edit inc/testing/stats.h
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
exe 'vert 1resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 362)
argglobal
balt src/testing/stats.cpp
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
let s:l = 126 - ((38 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 126
normal! 04|
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
let s:l = 260 - ((29 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 260
normal! 019|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/experiments.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
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
let s:l = 1 - ((0 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 362)
tabnext 1
badd +284 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +125 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +78 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +127 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +38 ~/Documents/master/pebbling-pdr/inc/io.h
badd +71 ~/Documents/master/pebbling-pdr/inc/testing/logger.h
badd +1 ~/Documents/master/pebbling-pdr/src/testing/logger.cpp
badd +249 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +81 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +0 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
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
