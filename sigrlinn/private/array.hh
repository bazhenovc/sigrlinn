#pragma once

#include <cstdint>
#include <cstddef>
#include <new>

namespace sgfx
{
namespace internal
{
///
/// ImmutableArray represents an abstract sequence container that cannot change in size.
///
/// It is guaranteed that all the data in this container uses contiguous storage locations for
/// its elements, which means that the elements can also be accessed using offsets on regular
/// pointers to its elements, and just as efficiently as in C arrays.
///
template <typename T>
class ImmutableArray
{
protected:

	size_t capacity  = 0;
	size_t size      = 0;

	T* pointer = nullptr;

	ImmutableArray() {}
	ImmutableArray(const ImmutableArray& other) = delete;
	virtual ~ImmutableArray() {}

public:

	inline T*       GetData()       { return pointer; }
	inline const T* GetData() const { return pointer; }

	inline       T& operator[](size_t index)       { return pointer[index]; }
	inline const T& operator[](size_t index) const { return pointer[index]; }

	// range for support
	inline T* begin() const { return pointer; }
	inline T* end()   const { return pointer + size; }

	// size and capacity
	inline size_t GetSize() const     { return size; }
	inline size_t GetCapacity() const { return capacity; }

	// utils
	inline bool IsEmpty() const { return size != 0; }

	inline ptrdiff_t Find(const T& e)
	{
		for (size_t i = 0; i < GetSize(); ++i)
			if (pointer[i] == e)
				return i;
		return -1;
	}
};

///
/// DynamicArray is a concrete sequence container representing a contiguous array that can change in
/// size.
///
/// Just like ImmutableArray, DynamicArray uses contiguous storage locations for its elements.
/// But unlike immutable arrays, dynamic array size can change dynamically, with its storage being
/// handled automatically by the container.
///
/// Internally, DynamicArray uses a dynamically allocated array to store its elements. This array
/// may need to be reallocated in order to grow in size when new elements are inserted, which
/// implies allocating a new array and moving all elements to it. This is a relatively expensive
/// task in terms of processing time, and thus, dynamic arrays do not reallocate each time an
/// element is added to the container, instead DynamicArray::kGrowAmount is used to control growing
/// size.
///
/// DynamicArray is very efficient with random access pattern, while adding and removing elements
/// is less effective compared to List.
///
template <typename T, size_t I = 32, size_t G = 64>
class DynamicArray final : public ImmutableArray<T>
{
protected:

	using ImmutableArray<T>::capacity;
	using ImmutableArray<T>::size;
	using ImmutableArray<T>::pointer;

private:

	enum
	{
		kInplaceStorageSize = I,
		kGrowAmount         = G
	};

	uint8_t _inplaceStorage[kInplaceStorageSize * sizeof(T)];

	inline void DeleteContents()
	{
		uint8_t* ptr = reinterpret_cast<uint8_t*>(pointer);
		if (ptr != _inplaceStorage) {
			delete [] ptr;
			pointer = reinterpret_cast<T*>(_inplaceStorage);
		}
	}

public:

	using ImmutableArray<T>::GetData;
	using ImmutableArray<T>::IsEmpty;
	using ImmutableArray<T>::Find;

	inline DynamicArray()
	{
		capacity = kInplaceStorageSize;
		pointer = reinterpret_cast<T*>(_inplaceStorage);
	}

	inline DynamicArray(const DynamicArray& other)
	{
		Resize(other.GetSize());
		for (size_t i = 0; i < size; ++i)
			::new (&pointer[i]) T(other[i]);
	}

	inline DynamicArray& operator=(const DynamicArray& other)
	{
		Resize(other.GetSize());
		for (size_t i = 0; i < size; ++i)
			::new (&pointer[i]) T(other[i]);
		return *this;
	}

	inline DynamicArray& operator=(const ImmutableArray<T>& other)
	{
		Resize(other.GetSize());
		for (size_t i = 0; i < size; ++i)
			::new (&pointer[i]) T(other[i]);
		return *this;
	}

	inline ~DynamicArray()
	{
		Purge();
	}

	inline void Clear()
	{
		for (size_t i = 0; i < size; ++i)
			pointer[i].~T();
		size = 0;
	}

	inline void Purge()
	{
		Clear();
		DeleteContents();

		size = 0;
		capacity = kInplaceStorageSize;
	}

	inline void Resize(size_t newSize)
	{
		if (newSize == size) return; // fool protection

		if (newSize > capacity) {
			Grow(newSize - size);
		}

		if (newSize < size) {
			for (size_t i = newSize; i < size; ++i)
				pointer[i].~T();
		} else {
			for (size_t i = size; i < newSize; ++i)
				::new (&pointer[i]) T();
		}
		size = newSize;
	}

	inline void Reserve(size_t numElements)
	{
		if (numElements > capacity)
			Grow(numElements - capacity);
	}

	inline void Grow(size_t numElements)
	{
		size_t newCapacity = size + numElements;
		capacity = newCapacity;

		if (newCapacity > kInplaceStorageSize) {
			uint8_t* newPointer = new uint8_t[newCapacity * sizeof(T)];
			T* ptr = reinterpret_cast<T*>(newPointer);

			for (size_t i = 0; i < size; ++i)
				::new (&ptr[i]) T(static_cast<T&&>(pointer[i]));

			DeleteContents();

			pointer = ptr;
		}
	}

	inline void Add(const T& element)
	{
		if (size >= capacity)
			Grow(kGrowAmount);

		T* ptr = pointer + size;
		::new (ptr) T(element);
		size ++;
	}

	template <typename ...Args>
	inline void EmplaceAdd(Args&&... args)
	{
		if (size >= capacity)
			Grow(kGrowAmount);

		T* ptr = pointer + size;
		::new (ptr) T(static_cast<Args&&>(args)...);
		size ++;
	}

	inline void Remove(size_t index)
	{
		if (index < size) {
			pointer[index].~T();

			T* ptr = pointer + index;
			for (size_t i = index + 1; i < size; ++i) {
				::new (ptr) T(static_cast<T&&>(pointer[i]));
				ptr++;
				ptr->~T();
			}
		}
	}

	inline void Remove(const T& element)
	{
		ptrdiff_t index = Find(element);
		if (index != -1)
			Remove(index);
	}
};

}
}
