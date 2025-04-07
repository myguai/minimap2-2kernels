# minimap2-fpga: FPGA-accelerated Minimap2

This work presents an end-to-end integrated FPGA-accelerated solution based on minimap2-fpga(https://github.com/kisarur/minimap2-fpga).
This work fixed the data dependency and applied a HLS-RTL combined flow, achieving a 2 kernels implementation.



## Getting Started 

1. Install "Intel® PAC with Intel® Arria® 10 GX FPGA Acceleration Stack Version 1.2.1" on the system 

2. Use the commands below to download the GitHub repo and setup the environment (you may need to update the variables defined in `opencl/init_env.sh`, if they're not already pointing to the correct paths in your system).
```
source opencl/init_env.sh
```

3. Make the rtl lib for opencl to use
```
cd device/RTL
make clean
make lib
```

4. [OPTIONAL] If you use Intel Arria 10 GX FPGA device, for which our accelerator implementation (on this branch) is optimized, it is recommended that you use the already built FPGA hardware binary included with this repo at `bin/minimap2_opencl_2kernels.aocx`. However, if you want to build this binary from source (in `device/minimap2_opencl_2kernels.cl`), you can use the `generate_fpga_binary_2kernels.sh` script included in the repo.



5. Use the commands below to build the host application and run FPGA-accelerated Minimap2 with the relative arugments<sup>1</sup>.
```
make
./minimap2-fpga [minimap2 arguments]
(e.g. ./minimap2-fpga -x map-ont <absolute path to reference sequence> <absolute path to query sequence>)
```





[paf]: https://github.com/lh3/miniasm/blob/master/PAF.md
[sam]: https://samtools.github.io/hts-specs/SAMv1.pdf
[minimap]: https://github.com/lh3/minimap
[smartdenovo]: https://github.com/ruanjue/smartdenovo
[longislnd]: https://www.ncbi.nlm.nih.gov/pubmed/27667791
[gaba]: https://github.com/ocxtal/libgaba
[ksw2]: https://github.com/lh3/ksw2
[preprint]: https://arxiv.org/abs/1708.01492
[release]: https://github.com/lh3/minimap2/releases
[mappypypi]: https://pypi.python.org/pypi/mappy
[mappyconda]: https://anaconda.org/bioconda/mappy
[issue]: https://github.com/lh3/minimap2/issues
[k8]: https://github.com/attractivechaos/k8
[manpage]: https://lh3.github.io/minimap2/minimap2.html
[manpage-cs]: https://lh3.github.io/minimap2/minimap2.html#10
[doi]: https://doi.org/10.1093/bioinformatics/bty191
[smide]: https://github.com/nemequ/simde
[unimap]: https://github.com/lh3/unimap
[minimap2fpga]: https://github.com/kisarur/minimap2-fpga