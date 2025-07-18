#pragma once
#include "ObjectMemPool.h"
#include <string.h>
// ���������
template <int BITS>//BITS��ʾ���ҳ����ռ�Ķ�����λ��32-PAGE_SHIFT ��64-PAGE_SHIFT
class TCMalloc_PageMap1 {
private:
	static const int LENGTH = 1 << BITS;
	void** array_;
public:
	typedef uintptr_t Number;
	explicit TCMalloc_PageMap1()
	{
#if 0
		size_t requiredBytes = sizeof(void*) << BITS;
		size_t alignedSize = SizeClass::_AlignSize(requiredBytes, 1 << PAGE_SHIFT);
		array_ = (void**)systemalloc(alignedSize >> PAGE_SHIFT);
		memset(array_, 0, sizeof(void*) << BITS);
#endif		
	}
	// ����KEY�ĵ�ǰֵ�������û�����û���k������Χ������NULL��
	void* get(Number k) const {
		if ((k >> BITS) > 0) {
			return NULL;
		}
		return array_[k];
	}
	// Ҫ�� "k" �ڷ�Χ "[0,2^BITS-1]" �ڡ�
	// Ҫ�� "k" �Ѿ���֮ǰȷ������
	//
	// ���ü� 'k' ��ֵΪ 'v'��
	void set(Number k, void* v) {
		array_[k] = v;
	}
};
// ���������
template <int BITS>
class TCMalloc_PageMap2 {
private:
	// �ڸ��ڵ��32����Ŀ��ÿ��Ҷ�ڵ��(2^BITS)/32����Ŀ��
	static const int ROOT_BITS = 5;
	static const int ROOT_LENGTH = 1 << ROOT_BITS;
	static const int LEAF_BITS = BITS - ROOT_BITS;
	static const int LEAF_LENGTH = 1 << LEAF_BITS;
	// Ҷ�ڵ�
	struct Leaf {
		void* values[LEAF_LENGTH];
	};
	Leaf* root_[ROOT_LENGTH];// ָ��32���ӽڵ��ָ��
public:
	typedef uintptr_t Number;
	explicit TCMalloc_PageMap2()
	{
		//allocator_ = allocator;
		memset(root_, 0, sizeof(root_));
		PreallocateMoreMemory();
	}
	void* get(Number k) const {
		const Number i1 = k >> LEAF_BITS;
		const Number i2 = k & (LEAF_LENGTH - 1);
		if ((k >> BITS) > 0 || root_[i1] == NULL) {
			return NULL;
		}
		return root_[i1]->values[i2];
	}
	void set(Number k, void* v) {
		const Number i1 = k >> LEAF_BITS;
		const Number i2 = k & (LEAF_LENGTH - 1);
		assert(i1 < ROOT_LENGTH);
		root_[i1]->values[i2] = v;
	}
	bool Ensure(Number start, size_t n) {
		for (Number key = start; key <= start + n - 1;) {
			const Number i1 = key >> LEAF_BITS;
			// ������
			if (i1 >= ROOT_LENGTH)
				return false;
			// �����Ҫ�������ڶ���ڵ�
			if (root_[i1] == NULL) {
				static ObjectMemPool<Leaf> LeafPool;
				Leaf* leaf = LeafPool.New();
				if (leaf == NULL) return false;
				memset(leaf, 0, sizeof(*leaf));
				root_[i1] = leaf;
			}
			// �������������Ҷ�ڵ㸲�ǵ���һ������
			key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;
		}
		return true;
	}
	void PreallocateMoreMemory() {
		// �����㹻���ڴ����������п��ܵ�ҳ��
		Ensure(0, 1 << BITS);
	}
};
// ����������ṹ
template <int BITS>
class TCMalloc_PageMap3 {
private:
	// ÿ���ڲ������Ķ���λ
	static const int INTERIOR_BITS = (BITS + 2) / 3; // ����ȡ��
	static const int INTERIOR_LENGTH = 1 << INTERIOR_BITS;
	// Ҷ�Ӳ����Ķ���λ
	static const int LEAF_BITS = BITS - 2 * INTERIOR_BITS;
	static const int LEAF_LENGTH = 1 << LEAF_BITS;
	// �ڲ��ڵ�
	struct Node {
		Node* ptrs[INTERIOR_LENGTH]; // ָ����һ��ڵ��ָ��
	};
	// Ҷ�ӽڵ�
	struct Leaf {
		void* values[LEAF_LENGTH]; // ʵ�ʵ�����ֵ
	};
	Node* root_; // �������ĸ��ڵ�
	Node* NewNode() {
		static ObjectMemPool<Node> NodePool;
		Node* result = NodePool.New();
		if (result != NULL) {
			memset(result, 0, sizeof(*result));
		}
		return result;
	}
public:
	typedef uintptr_t Number; // ҳ�ŵ�����
	explicit TCMalloc_PageMap3()
	{
		//allocator_ = allocator;
		root_ = NewNode(); // �������ڵ�
	}
	void* get(Number k) const { // ����ҳ�Ż�ȡֵ
		const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS); // ��һ������
		const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1); // �ڶ�������
		const Number i3 = k & (LEAF_LENGTH - 1); // ����������

		if ((k >> BITS) > 0 || // ���ҳ���Ƿ񳬳���Χ
			root_->ptrs[i1] == NULL || root_->ptrs[i1]->ptrs[i2] == NULL)  // ���ڵ��Ƿ����
			return NULL; // ���û���ҵ����߳�����Χ������NULL
		
		return reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3]; // ����ҵ�������ֵ
	}
	void set(Number k, void* v) { // ����ҳ������ֵ
		assert(k >> BITS == 0); // ����ҳ���ڷ�Χ��
		Ensure(k, 1); //ȷ����kҳ��Ӧ�ڵ����,�������򿪱ٽڵ�ռ�
		const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS); // ��һ������
		const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1); // �ڶ�������
		const Number i3 = k & (LEAF_LENGTH - 1); // ����������
		reinterpret_cast<Leaf*>(root_->ptrs[i1]->ptrs[i2])->values[i3] = v; // ����ֵ
	}
	bool Ensure(Number start, size_t n) { // ȷ��һ��ҳ�ŷ�Χ�ڵĽڵ㶼����
		for (Number key = start; key <= start + n - 1;) 
		{ // ����ҳ�ŷ�Χ
			const Number i1 = key >> (LEAF_BITS + INTERIOR_BITS); // ��һ������
			const Number i2 = (key >> LEAF_BITS) & (INTERIOR_LENGTH - 1); // �ڶ�������
			// ���������
			if (i1 >= INTERIOR_LENGTH || i2 >= INTERIOR_LENGTH)
				return false; // ���������Χ������false
			// �����Ҫ�������ڶ���ڵ�
			if (root_->ptrs[i1] == NULL)
			{
				Node* n = NewNode(); // �����½ڵ�
				if (n == NULL) 
					return false; // ����ڴ����ʧ�ܣ�����false
				root_->ptrs[i1] = n; // ����ָ���½ڵ��ָ��
			}
			// �����Ҫ������Ҷ�ӽڵ�
			if (root_->ptrs[i1]->ptrs[i2] == NULL)
			{
				static ObjectMemPool<Leaf> LeafPool_3;
				Leaf* leaf = reinterpret_cast<Leaf*>(LeafPool_3.New());

				if (leaf == NULL) 
					return false; // ����ڴ����ʧ�ܣ�����false

				memset(leaf, 0, sizeof(*leaf)); // ��ʼ��Ҷ��Ϊ��
				root_->ptrs[i1]->ptrs[i2] = reinterpret_cast<Node*>(leaf); // ����ָ����Ҷ�ӵ�ָ��
			}
			// ��ҳ���ƽ�����һ��Ҷ�ӽڵ�ķ�Χ
			key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;
		}
		return true; // ����ɹ�������true
	}
	void PreallocateMoreMemory() {
		// ��ǰ����ĳ�οռ�������Ч�ʣ������Ҫ��ʵ���������
	}
};