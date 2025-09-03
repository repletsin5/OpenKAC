#include "Array.hpp"


template<typename T>
inline void OpenKAC::std::Array<T>::put(T value)
{


}

template<typename T>
bool OpenKAC::std::Array<T>::reallocate(UINT32 ammount)
{
	
	if (this->data_ == 0) {
		this->data_ = new T[2];
		this->allocatedSize_ = 2;
		if (this->data_ != 0) {
			//TODO store what error actuall occured and then able to get the error with e.g getLastKACerror;
			return true;
		}
		else {
			return false;
		}
	}


	if ((this->size_ + this->ammount) > this->allocatedSize_) {

		T* temp = this->data_;
		UINT32 tempSize = this->allocatedSize_;

		this->allocatedSize_ *= 2;
		this->data_ = new T[this->allocatedSize_];
		if(allocatedSize_ >)
	}
}
