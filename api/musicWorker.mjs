// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
// musicWorker.mjs for WorkerScript
import { kugouHandler } from './kugouapi.mjs';
import { neteaseHandler } from './necloudapi.mjs';

WorkerScript.onMessage = function(message) {
    // source: 0 = Kugou, 1 = NetEase, 2 = QQ (预留)
    const source = (message.source !== undefined) ? message.source : 0;

    // 回调
    message.sendMessage = WorkerScript.sendMessage;

    switch (source) {
    case 0:
        kugouHandler(message);
        break;
    case 1:
        neteaseHandler(message);
        break;
    default:
        console.log("musicWorker: 未知 source =", source);
    }
};
