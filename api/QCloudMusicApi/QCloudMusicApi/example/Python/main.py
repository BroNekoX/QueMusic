import json
import sys

sys.path.append('.')

from api import *

if __name__ == '__main__':
    setFilterRules("QCloudMusicApi.debug=false")

    result = invoke("lyric_new", json.dumps({
        "id": "2058263032"
    }))
    print("result", json.dumps(json.loads(result), indent = 4, ensure_ascii = False))
    
    lib.freeApi()