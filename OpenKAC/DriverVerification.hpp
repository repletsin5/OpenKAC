#pragma once

namespace OpenKAC {
    class DriverVerification {
    public:
        static void CheckWdFilter();
        static void CheckHashBucketList();
        static void TimingCheck();
        static void CheckTPM2dot0();
    };
}