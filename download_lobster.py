from huggingface_hub import snapshot_download

snapshot_download(
    repo_id="totalorganfailure/lobster-data",
    repo_type="dataset",
    allow_patterns=[
        "LOBSTER_SampleFile_AAPL_2012-06-21_10/*"
    ],
    local_dir="data/lobster"
)

print("Download completed!")