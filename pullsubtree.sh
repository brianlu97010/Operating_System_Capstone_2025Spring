# Usage: ./pullsubtree.sh -lab [lab_num]
# 檢查參數
if [[ "$1" != "-lab" || -z "$2" ]]; then
    echo "Usage: ./pullsubtree.sh -lab [1-7]"
    exit 1
fi

# 讀取 lab 編號
lab_num=$2

git subtree pull --prefix="lab${lab_num}" git@github.com:osc2025/"lab${lab_num}"-brianlu97010.git main
