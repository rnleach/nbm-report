set terminal pngcairo

site = 'kmso'
#element = 'min_temp'
#element = 'max_temp'
element = 'precip'

pdfs = sprintf('%s_%s_pdfs.dat', site, element)
cdfs = sprintf('%s_%s_cdfs.dat', site, element)
scenarios = sprintf('%s_%s_scenarios.dat', site, element)

set ytics nomirror

stats pdfs using 1 prefix "T"
do for [i=0:T_blocks] {
    set output sprintf('%s_%s_pdfs_%02d.png',site, element, i)
    set title site." ".element
    plot [T_min:T_max]                                                               \
         cdfs      index i u 1:2          axis x1y1 w l lw 1      t columnhead(1),   \
         pdfs      index i u 1:2          axis x1y2 w l lw 2      t "PDF",           \
         scenarios index i u 2:($4 * 100) axis x1y1 w p ps 3 pt 7 t "Modes",         \
         scenarios index i u 1:($4 * 100) axis x1y1 w p ps 3      t "Mins"
}

