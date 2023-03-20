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
$argadd src/model/pdr/pdr-context.cpp
edit src/model/pdr/pdr-context.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
2wincmd h
wincmd _ | wincmd |
split
1wincmd k
wincmd w
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
exe '1resize ' . ((&lines * 54 + 43) / 87)
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe '2resize ' . ((&lines * 29 + 43) / 87)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 4resize ' . ((&columns * 121 + 181) / 363)
argglobal
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
let s:l = 41 - ((37 * winheight(0) + 27) / 54)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 41
normal! 03|
wincmd w
argglobal
if bufexists("inc/tactic.h") | buffer inc/tactic.h | else | edit inc/tactic.h | endif
if &buftype ==# 'terminal'
  silent file inc/tactic.h
endif
balt src/model/pdr/pdr-context.cpp
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
let s:l = 21 - ((20 * winheight(0) + 14) / 29)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 21
normal! 054|
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
let s:l = 90 - ((24 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 90
normal! 048|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
endif
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
let s:l = 161 - ((46 * winheight(0) + 42) / 84)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 161
normal! 021|
lcd ~/Documents/master/pebbling-pdr
wincmd w
4wincmd w
exe '1resize ' . ((&lines * 54 + 43) / 87)
exe 'vert 1resize ' . ((&columns * 120 + 181) / 363)
exe '2resize ' . ((&lines * 29 + 43) / 87)
exe 'vert 2resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 3resize ' . ((&columns * 120 + 181) / 363)
exe 'vert 4resize ' . ((&columns * 121 + 181) / 363)
tabnext 1
badd +50 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-context.cpp
badd +50 ~/Documents/master/pebbling-pdr/src/algo/generalize.cpp
badd +158 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +143 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +1 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +0 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
badd +21 ~/Documents/master/pebbling-pdr/inc/tactic.h
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
nohlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
