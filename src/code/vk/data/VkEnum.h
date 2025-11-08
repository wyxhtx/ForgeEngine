#pragma once



namespace bufferEnum {
    namespace textAlign {
        enum Type {
            alignLeft,
            alignCenter,
            alignRight
        };
    }
}

namespace pipelineEnum {
    namespace terrain {
        enum {
            Lod_0,              //无方块合并，可破坏
            Lod_1,              //无方块合并，不可破坏

            count				//枚举长度
        };
    }
}