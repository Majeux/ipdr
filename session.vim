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
$argadd src/testing/experiments.cpp
edit src/testing/experiments.cpp
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
exe 'vert 2resize ' . ((&columns * 120 + 181) / 363)
exe '3resize ' . ((&lines * 41 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 121 + 181) / 363)
exe '4resize ' . ((&lines * 42 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 121 + 181) / 363)
argglobal
balt src/testing/pebbling-experiments.cpp
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
let s:l = 186 - ((60 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 186
normal! 020|
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
let s:l = 222 - ((49 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 222
normal! 05|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson-result.h
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
let s:l = 71 - ((26 * winheight(0) + 20) / 41)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 71
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson-result.h
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
let s:l = 40 - ((24 * winheight(0) + 21) / 42)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 40
normal! 054|
lcd ~/Documents/master/pebbling-pdr
wincmd w
2wincmd w
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 363)
exe '3resize ' . ((&lines * 41 + 43) / 87)
exe 'vert 3resize ' . ((&columns * 121 + 181) / 363)
exe '4resize ' . ((&lines * 42 + 43) / 87)
exe 'vert 4resize ' . ((&columns * 121 + 181) / 363)
tabnext 1
badd +228 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +115 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +30 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +216 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +113 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +207 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +25 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +21 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +136 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +639 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +399 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +69 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
badd +70 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +40 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp
badd +34 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson-result.h
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
