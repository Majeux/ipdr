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
edit src/testing/stats.cpp
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
wincmd _ | wincmd |
split
1wincmd k
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
exe 'vert 2resize ' . ((&columns * 128 + 181) / 363)
exe '3resize ' . ((&lines * 38 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 113 + 181) / 363)
exe '4resize ' . ((&lines * 45 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 113 + 181) / 363)
argglobal
balt inc/testing/stats.h
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
let s:l = 625 - ((67 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 625
normal! 049|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/stats.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/stats.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/stats.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/auxiliary/math.h
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
let s:l = 396 - ((4 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 396
normal! 013|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/frames.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/frames.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
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
let s:l = 509 - ((19 * winheight(0) + 19) / 38)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 509
normal! 022|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/experiments.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
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
let s:l = 226 - ((36 * winheight(0) + 22) / 45)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 226
normal! 060|
lcd ~/Documents/master/pebbling-pdr
wincmd w
2wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 128 + 181) / 363)
exe '3resize ' . ((&lines * 38 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 113 + 181) / 363)
exe '4resize ' . ((&lines * 45 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 113 + 181) / 363)
tabnext 1
badd +474 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +157 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +113 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +30 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +43 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +226 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +86 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +281 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +41 ~/Documents/master/pebbling-pdr/inc/auxiliary/math.h
badd +186 ~/Documents/master/pebbling-pdr/src/testing/peterson-experiments.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +81 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +290 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +38 ~/Documents/master/pebbling-pdr/inc/io.h
badd +59 ~/Documents/master/pebbling-pdr/inc/testing/logger.h
badd +1 ~/Documents/master/pebbling-pdr/src/testing/logger.cpp
badd +39 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +48 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +230 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +31 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
badd +294 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
badd +68 ~/Documents/master/pebbling-pdr/inc/testing/experiments.h
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
