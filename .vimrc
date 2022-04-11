if SpmSetProject("ConMan") == 0
    nmap <leader>a :w<CR>:!pio run<CR>
    nmap <leader>u :w<CR>:!pio run -t upload<CR>
    nmap <leader>h yy'fpwd2f:f)lC;<esc>==mf^w
    nmap <leader>s 02w"aye2b02w"byej/::aVf{%jd/::bf{%jpzz^wj
endif
