#include <Defs.h>

namespace OpenKAC::std {


	template<typename T>
	class Array {

	public:
		T* data() { return this.data_; }
		UINT32 size() { return this.size_; }

		Array() {};

		void put(T value);
	private:

		bool reallocate(UINT32 ammount);

		T* data_ = 0;
		UINT32 size_ = 0;
		UINT32 allocatedSize_ = 0;
	};
};