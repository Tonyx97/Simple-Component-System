#pragma once

void debug()
{
	while (true)
	{
		PROCESS_MEMORY_COUNTERS_EX data;
		GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&data), sizeof(data));
		float mb = static_cast<float>(data.WorkingSetSize) / powf(2.f, 20.f),
			  kb = static_cast<float>(data.WorkingSetSize) / powf(2.f, 10.f);

		char buf[64] = { 0 };
		sprintf_s(buf, "Memory: %.3f MB | %.3f KB", mb, kb);
		SetConsoleTitleA(buf);
		Sleep(100);
	}
}