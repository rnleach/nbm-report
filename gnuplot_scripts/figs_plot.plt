set terminal pngcairo

set ytics nomirror

site = 'kmso'
site_title = "KMSO"
unset ytics

element = 'min_temp'
pdfs = sprintf('%s_%s_pdfs.dat', site, element)
cdfs = sprintf('%s_%s_cdfs.dat', site, element)
scenarios = sprintf('%s_%s_scenarios.dat', site, element)
stats pdfs using 1 prefix "T"
do for [i=0:T_blocks - 1] {
    set output sprintf('%s_%s_pdfs_%02d.png',site, element, i)
    set title sprintf('%s Minimum Temperature', site_title)
    set xlabel "Temperature (F)"
    set format x "%3.0f"
    plot [T_min:T_max]                                                                      \
         cdfs      index i u 1:2          axis x1y1 w l lw 1      t "CDF ".columnhead(1),   \
         pdfs      index i u 1:2          axis x1y2 w l lw 2      t "PDF"
}

element = 'max_temp'
pdfs = sprintf('%s_%s_pdfs.dat', site, element)
cdfs = sprintf('%s_%s_cdfs.dat', site, element)
scenarios = sprintf('%s_%s_scenarios.dat', site, element)

stats pdfs using 1 prefix "T"
do for [i=0:T_blocks - 1] {
    set output sprintf('%s_%s_pdfs_%02d.png',site, element, i)
    set title sprintf('%s Maximum Temperature', site_title)
    set xlabel "Temperature (F)"
    set format x "%3.0f"
    plot [T_min:T_max]                                                                      \
         cdfs      index i u 1:2          axis x1y1 w l lw 1      t "CDF ".columnhead(1),   \
         pdfs      index i u 1:2          axis x1y2 w l lw 2      t "PDF"
}

element = 'snow'
pdfs = sprintf('%s_%s_pdfs.dat', site, element)
cdfs = sprintf('%s_%s_cdfs.dat', site, element)
scenarios = sprintf('%s_%s_scenarios.dat', site, element)

stats pdfs using 1 prefix "T"
do for [i=0:T_blocks - 1] {
    set output sprintf('%s_%s_pdfs_%02d.png',site, element, i)
    set title sprintf('%s 24-hour Snow Accumulation', site_title)
    set xlabel "Inches"
    set format x "%4.1f"
    plot [T_min:T_max]                                                                      \
         cdfs      index i u 1:2          axis x1y1 w l lw 1      t "CDF ".columnhead(1),   \
         pdfs      index i u 1:2          axis x1y2 w l lw 2      t "PDF"
}

element = 'precip'
pdfs = sprintf('%s_%s_pdfs.dat', site, element)
cdfs = sprintf('%s_%s_cdfs.dat', site, element)
scenarios = sprintf('%s_%s_scenarios.dat', site, element)

stats pdfs using 1 prefix "T"
do for [i=0:T_blocks - 1] {
    set output sprintf('%s_%s_pdfs_%02d.png',site, element, i)
    set title sprintf('%s 24-hour Precipitation Accumulation', site_title)
    set xlabel "Inches"
    set format x "%4.2f"
    plot [T_min:T_max]                                                                      \
         cdfs      index i u 1:2          axis x1y1 w l lw 1      t "CDF ".columnhead(1),   \
         pdfs      index i u 1:2          axis x1y2 w l lw 2      t "PDF"
}

