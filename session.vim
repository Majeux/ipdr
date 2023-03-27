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
$argadd src/algo/frames.cpp
set stal=2
tabnew
tabrewind
edit src/algo/ipdr-pebbling.cpp
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
exe 'vert 1resize ' . ((&columns * 117 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 109 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 135 + 181) / 363)
argglobal
balt src/algo/solver.cpp
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
let s:l = 178 - ((24 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 178
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/cli-parse.cpp") | buffer ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | else | edit ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
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
let s:l = 247 - ((0 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 247
normal! 029|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/cli-parse.cpp") | buffer ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | else | edit ~/Documents/master/pebbling-pdr/src/cli-parse.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
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
let s:l = 556 - ((0 * winheight(0) + 41) / 83)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 556
normal! 025|
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe 'vert 1resize ' . ((&columns * 117 + 181) / 363)
exe 'vert 2resize ' . ((&columns * 109 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 135 + 181) / 363)
tabnext
edit ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
argglobal
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
let s:l = 190 - ((31 * winheight(0) + 41) / 82)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 190
normal! 020|
lcd ~/Documents/master/pebbling-pdr
tabnext 1
set stal=1
badd +378 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +25 ~/Documents/master/pebbling-pdr/src/algo/frames.cpp
badd +190 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +151 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +218 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +251 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +75 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +51 ~/Documents/master/pebbling-pdr/src/algo/solver.cpp
badd +210 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +80 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +141 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
badd +1 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-context.h
badd +51 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +177 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +51 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +30 ~/Documents/master/pebbling-pdr/inc/tactic.h
badd +1 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +50 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +41 ~/Documents/master/pebbling-pdr/inc/algo/frames.h
badd +12 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
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
