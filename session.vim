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
exe '3resize ' . ((&lines * 34 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 113 + 181) / 363)
exe '4resize ' . ((&lines * 49 + 43) / 87)
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
let s:l = 209 - ((34 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 209
normal! 018|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/stats.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/stats.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/stats.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
endif
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
let s:l = 397 - ((41 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 397
normal! 026|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/experiments.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
endif
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
let s:l = 228 - ((28 * winheight(0) + 17) / 34)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 228
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/inc/cli-parse.h") | buffer ~/Documents/master/pebbling-pdr/inc/cli-parse.h | else | edit ~/Documents/master/pebbling-pdr/inc/cli-parse.h | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/inc/cli-parse.h
endif
balt ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
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
let s:l = 72 - ((0 * winheight(0) + 24) / 49)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 72
normal! 023|
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 128 + 181) / 363)
exe '3resize ' . ((&lines * 34 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 113 + 181) / 363)
exe '4resize ' . ((&lines * 49 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 113 + 181) / 363)
tabnext 1
badd +397 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +57 ~/Documents/master/pebbling-pdr/inc/auxiliary/math.h
badd +200 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +45 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +281 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +209 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +254 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
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
