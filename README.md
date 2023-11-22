# dataset-dinov2

This program is meant to perform the download, decompression, unarchiving of the Imagenet1k dataset as a piped process using a memory buffer. \
For now it only works with the required filestructure requirements of https://github.com/facebookresearch/dinov2#data-preparation but could be extended to work with any filestructure requirements.

My use case was setting up https://github.com/facebookresearch/dinov2#data-preparation, my connected EFS mount targets were getting saturated when each step would retrieve from the filesystem. So i wrote this C program to perform the steps on the fly. 

I have a container image for this program [] along with YAML job declarations to perform this operation on a kubernetes cluster. Automating this process allows for bringing up the dataset quickly (~2h) when needed for a training job on the cluster \ since download of large datasets to AWS are free but storage costs money.

# To build
```bash
cd Inmemory-dataset 
git clone https://github.com/ccharest93/tarlib.git 
# now follow instruction on repo to build tarlib 
mkdir build && cd build 
cmake .. 
make
```

# Usage
```bash
./dataset-dinov2 <input_trace_filenmae> <processing case> <hf_token> <output_dir>
```
