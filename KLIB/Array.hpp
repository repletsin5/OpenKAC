#include "std_includes.h"

namespace klib::std {


	template<typename T>
	class array {
	private:

		bool reallocate(UINT32 ammount);

		T* data_ = 0;
		UINT32 size_ = 0;
		UINT32 allocatedSize_ = 0;
	public:
		array(UINT32 size);
		~array();
		void setCount(UINT32 size) { this->size_ = size; }

		T* data() { return this->data_; }
		UINT32 count() { return this->size_; }
		UINT32 allocatedSize() { return this->allocatedSize_; }
		UINT32 allocatedSizeBytes() { return this->allocatedSize_ * sizeof(T); }

		array() {};
		T* begin() { return &data_[0]; }
		T* end() { return &data_[size_]; }

		//const T* begin() const { 
		//	return (const T*)(this->data());
		//}
		//const T* end() const { return ((const T*))((UINT64)(data() + allocatedSizeBytes()); }

		void put(T value);

	};
};

template<typename T>
inline klib::std::array<T>::array(UINT32 size)
{

	allocatedSize_ = size;
	data_ = new T[size];
}

template<typename T>
inline klib::std::array<T>::~array()
{
	auto p = data_;
	data_ = 0;
	allocatedSize_ = 0;
	size_ = 0;
	delete[] p;
}

template<typename T>
inline void klib::std::array<T>::put(T value)
{


}

template<typename T>
bool klib::std::array<T>::reallocate(UINT32 ammount)
{

	//if (this->data_ == 0) {
	//	this->data_ = new T[2];
	//	this->allocatedSize_ = 2;
	//	if (this->data_ != 0) {
	//		//TODO store what error actuall occured and then able to get the error with e.g getLastKACerror;
	//		return true;
	//	}
	//	else {
	//		return false;
	//	}
	//}


	//if ((this->size_ + this->ammount) > this->allocatedSize_) {

	//	T* temp = this->data_;
	//	UINT32 tempSize = this->allocatedSize_;

	//	this->allocatedSize_ *= 2;
	//	this->data_ = new T[this->allocatedSize_];
	//	//if(allocatedSize_ >)
	//}

	return false;
}