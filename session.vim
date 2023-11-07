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
$argadd src/pebbling-pdr.cpp
edit src/model/pebbling/pebbling-result.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
3wincmd h
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
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 88 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 92 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 89 + 181) / 362)
argglobal
balt src/pebbling-pdr.cpp
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
let s:l = 185 - ((47 * winheight(0) + 39) / 78)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 185
normal! 019|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp") | buffer ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp | else | edit ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/algo/result.cpp
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
let s:l = 302 - ((57 * winheight(0) + 39) / 78)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 302
normal! 0
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
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
let s:l = 39 - ((38 * winheight(0) + 39) / 78)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 39
normal! 031|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/algo/bounded.cpp") | buffer ~/Documents/master/pebbling-pdr/src/algo/bounded.cpp | else | edit ~/Documents/master/pebbling-pdr/src/algo/bounded.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/algo/bounded.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/algo/bounded.h
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
let s:l = 183 - ((44 * winheight(0) + 39) / 78)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 183
normal! 028|
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 88 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 92 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 89 + 181) / 362)
tabnext 1
badd +272 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +102 ~/Documents/master/pebbling-pdr/inc/algo/bounded.h
badd +141 ~/Documents/master/pebbling-pdr/src/algo/bounded.cpp
badd +66 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +33 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +32 ~/Documents/master/pebbling-pdr/inc/algo/obligation.h
badd +127 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +111 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +82 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +32 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +39 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +169 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +37 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-result.h
badd +74 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-result.cpp
badd +95 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
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
