# 插件市场

把插件仓库打包成静态目录，应用从 `index.json` 拉列表、按需下载 zip 安装。

## 1. 打包

```bash
python tools/market/build_market.py \
    --plugins path/to/QuePlugins \
    --out build/market
```

产物：

```
build/market/
  index.json
  packages/<id>.zip
  preview/<id>.png
```

插件信息取自各处 `info.json` 的 `name` / `type` / `version` / `preview`，
`sha256` 与 `size` 由脚本计算，应用下载后会校验。

## 2. 本地试

```bash
cd build/market
python -m http.server 8765
```

应用 → 设置 → 插件 → 插件市场 → 右上角「市场设置」→ 服务器地址填
`http://127.0.0.1:8765` → 保存。索引地址会自动拼成 `…/market/index.json`；
直接填以 `.json` 结尾的完整地址也可以。

打包出的 `packages/<id>.zip` 也可以直接发给别人，用「设置 → 插件 → 导入 ZIP」本地安装。

## 3. 部署到服务器

服务器（8.133.18.144）上把 `build/market` 整个目录放到静态站点根下：

```bash
# 本地
scp -r build/market/* root@8.133.18.144:/var/www/html/market/

# 服务器（nginx，仅作参考）
# location /market/ { alias /var/www/html/market/; expires 1h; }
```

应用默认服务器地址就是 `http://8.133.18.144/`（索引 = `…/market/index.json`），
服务器起来后不用改客户端配置。

建议加个 systemd timer 或 CI，定时重新执行 `build_market.py` 再同步，
插件仓库更新后市场自动跟着更新。

## 4. index.json 格式

```json
{
  "version": 1,
  "plugins": [
    {
      "id": "now-playing",
      "name": "悬浮播放卡",
      "type": "function",
      "author": "QueMusic",
      "version": "1.0.0",
      "description": "右下角悬浮卡片",
      "updated": "2026-11-13 18:00",
      "preview": "preview/now-playing.png",
      "download": "packages/now-playing.zip",
      "sha256": "…",
      "size": 2626
    }
  ]
}
```

- `preview` / `download` 支持相对路径（相对 `index.json`），也支持绝对 URL。
- `type` 取 `lyrics` / `function` / `source`（音乐源），与插件自身 `info.json` 一致。
- `updated` 用于「最新」排序（`YYYY-MM-DD HH:MM`）。

## 5. 客户端行为

- 打开市场页自动刷新一次；右上角「刷新」手动重拉。
- 分类固定 5 个：推荐（全部随机）/ 最新（按 `updated`）/ 歌词界面 / 功能 / 音乐源，本地切换不重新请求。
- 下载 → 校验 sha256 → 解压 → 校验 `info.json` → 交给宿主安装；
  任一步失败都会提示原因，不会留下半个插件。
- 安装完成后按钮变为「已安装」；市场版本更高时显示「更新」。