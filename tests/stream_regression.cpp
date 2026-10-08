#include <cstdio>
#include <cstring>
#include <vector>
#include "common.h"
#include "TS_BUFF.h"
#include "tswriter.h"
FILE* g_fpLog = NULL;
class WriterProbe : public CTsWriter {
public:
    WriterProbe(HRESULT* hr) : CTsWriter(L"synthetic", NULL, hr) {}
    using CTsWriter::Write;
};
static int CALLBACK Receive(void* param, BYTE* data, size_t size) {
    static_cast<TS_BUFF*>(param)->AddData(data, size);
    return 0;
}
int main() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT hr = S_OK;
    WriterProbe writer(&hr);
    TS_BUFF queue;
    queue.SetSize(188, 8);
    writer.SetCallBackRecv(Receive, &queue);
    std::vector<BYTE> expected(188 * 3);
    for (size_t i = 0; i < expected.size(); ++i) expected[i] = static_cast<BYTE>(i);
    // 187/1 byte fragments must not disappear before reaching the assembler.
    writer.Write(expected.data(), 187);
    writer.Write(expected.data() + 187, 1);
    writer.Write(expected.data() + 188, 376);
    std::vector<BYTE> actual;
    while (TS_DATA* item = queue.Get()) {
        actual.insert(actual.end(), item->pbyBuff, item->pbyBuff + item->Size);
        delete item;
    }
    writer.SetCallBackRecv(NULL, NULL);
    if (actual != expected) {
        std::printf("FAIL callback_fragment_exact expected=%zu actual=%zu\n", expected.size(), actual.size());
        return 1;
    }
    std::puts("PASS callback_fragment_exact");
    queue.SetSize(188, 2);
    std::vector<BYTE> burst(188 * 6, 0x47);
    queue.AddData(burst.data(), burst.size());
    unsigned long long dropped = 0;
    dropped = queue.GetDroppedBytes();
    if (queue.Size() != 2 || dropped != 188 * 4) {
        std::printf("FAIL bounded_overflow_observable size=%zu dropped=%llu\n", queue.Size(), dropped);
        return 2;
    }
    std::puts("PASS bounded_overflow_observable");
    queue.SetSize(188, 0);
    queue.AddData(burst.data(), 188);
    if (queue.Size() != 1) { std::puts("FAIL zero_capacity_safe"); return 3; }
    std::puts("PASS zero_capacity_safe");
    CoUninitialize();
    return 0;
}
