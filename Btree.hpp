//   518021910882
//   杜劲达


//   B+树优越性：由于中间节点比叶子结点信息量少，B+树高度较小，每次查询修改IO次数少，但需要到叶子结点，因此随机查询性能稳定，较快
//               由于叶子结点之间有链接关联，范围查询与遍历性能优越

#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <map>
#include <fstream>
#include <cstdio>
#include <cstring>
using namespace std;
namespace sjtu {
	template<class Key, class Value, class Compare = std::less<Key> >
	class BTree {
        //static const int M = (4000 - 2 * sizeof(size_t) - sizeof(bool) - sizeof(int)) / (sizeof(Key) + sizeof(size_t)) - 1;
        //static const int L = (4000 - 4 * sizeof(size_t) - sizeof(int)) / (sizeof(Key) + sizeof(size_t)) - 1;
		static const int M = 228;
		static const int L = 32;

		/*=========================文件操作=============================*/
		void readFile(void *place, size_t offset, int num, int size) {
			fseek(file, offset, 0);
			fread(place, num, size, file);
			fflush(file);
		}

		void writeFile(void *place, size_t offset, int num, int size) {
			fseek(file, offset, 0);
			fwrite(place, num, size, file);
			fflush(file);
		}
		/*=========================文件操作=============================*/

	private:
		// Your private members go here
		FILE *file;
		char name[20];

	public:

		struct basic_information {
			size_t head;
			size_t tail;
			size_t tree_size;
			size_t root;
			size_t end;

			basic_information() {
				head = 0;
				tail = 0;
				tree_size = 0;
				root = 0;
				end = 0;
			}
		};

		basic_information basic_info;

		struct leaf_node {
			int parent;
			int prev;
			int next;
			int num;
			int offset;

			pair<Key, Value> data[L + 1];                    //叶子节点下管一串数据（不算最后一层，最后一层算卫星数据）

			leaf_node(int t = 0) {
				parent = 0;
				prev = 0;
				next = 0;
				num = 0;
				offset = t;
				memset(data, 0, L + 1);
			}
		};

		struct inter_node {
			int parent;
			int num;
			int offset;
			bool son_type;     //儿子节点是 1->叶节点   0->中间节点
			int son[M + 1];
			Key data[M + 1];

			inter_node(int t = 0) {
				offset = t;
				parent = 0;
				num = 0;
				son_type = 0;
				memset(son, 0, M + 1);
				memset(data, 0, M + 1);
			}
		};

		class const_iterator;

		class iterator {
			friend class BTree;

		private:
			// Your private members go here
			BTree *pt;
			int offset;

		public:
			bool modify(const Value &value) {
				return true;
			}

			iterator() {
				// TODO Default Constructor
				pt = nullptr;
				offset = 0;
			}

			iterator(BTree *b, int p = 0) {
				pt = nullptr;
				offset = p;
			}

			iterator(const iterator &other) {
				// TODO Copy Constructor
				pt = other.pt;
				offset = other.offset;
			}

			// Return a new iterator which points to the n-next elements
			iterator operator++(int) {
				// Todo iterator++
			}

			iterator &operator++() {
				// Todo ++iterator
			}

			iterator operator--(int) {
				// Todo iterator--
			}

			iterator &operator--() {
				// Todo --iterator
			}

			// Overloaded of operator '==' and '!='
			// Check whether the iterators are same
			bool operator==(const iterator &rhs) const {
				// Todo operator ==
			}

			bool operator==(const const_iterator &rhs) const {
				// Todo operator ==
			}

			bool operator!=(const iterator &rhs) const {
				// Todo operator !=
			}

			bool operator!=(const const_iterator &rhs) const {
				// Todo operator !=
			}
		};

		class const_iterator {
			// it should has similar member method as iterator.
			//  and it should be able to construct from an iterator.
		private:
			// Your private members go here
		public:
			const_iterator() {
				// TODO
			}

			const_iterator(const const_iterator &other) {
				// TODO
			}

			const_iterator(const iterator &other) {
				// TODO
			}
			// And other methods in iterator, please fill by yourself.
		};

		// Default Constructor and Copy Constructor
		BTree() {
			strcpy(name, "WoodJedi.txt");
			file = fopen(name, "rb+");

			if (file == nullptr) {
				file = fopen(name, "wb+");
				basic_info.tree_size = 0;
				basic_info.end = sizeof(basic_information);    //基本信息

				inter_node root(basic_info.end);
				root.num = 1;
				root.son_type = 1;
				basic_info.end += sizeof(inter_node);     //根节点
				basic_info.root = root.offset;

				leaf_node leaf(basic_info.end);
				basic_info.end += sizeof(leaf_node);       //第一个叶节点
				basic_info.head = basic_info.tail = leaf.offset;

				leaf.parent = root.offset;
				root.son[0] = leaf.offset;

				writeFile(&basic_info, 0, 1, sizeof(basic_information));
				writeFile(&root, root.offset, 1, sizeof(inter_node));
				writeFile(&leaf, leaf.offset, 1, sizeof(leaf_node));
			}
			else {
				readFile(&basic_info, 0, 1, sizeof(basic_information));
			}
			fflush(file);
		}

		BTree(const BTree &other) {}

		BTree &operator=(const BTree &other) {}

		~BTree() {
			// Todo Destructor
			fclose(file);
		}

		void clear() {
			basic_info.tree_size = 0;
			basic_info.end = sizeof(basic_information);    //基本信息

			inter_node root(basic_info.end);
			root.son_type = 1;
			root.num = 1;
			basic_info.root = root.offset;
			basic_info.end += sizeof(inter_node);     //根节点

			leaf_node leaf(basic_info.end);
			basic_info.head = basic_info.tail = leaf.offset;
			basic_info.end += sizeof(leaf_node);       //第一个叶节点

			root.son[0] = leaf.offset;
			leaf.parent = root.offset;

			writeFile(&basic_info, 0, 1, sizeof(basic_information));
			writeFile(&root, root.offset, 1, sizeof(inter_node));
			writeFile(&leaf, leaf.offset, 1, sizeof(leaf_node));
		}

		pair<iterator, OperationResult> insert(const Key &key, const Value &value) {
			int leafOffset = find_pos(key, basic_info.root);
			leaf_node leaf;

			if (basic_info.tree_size == 0 || leafOffset == 0) {     //简化tree_size为0和key最小情况
				OperationResult t = min_insert(leaf, leafOffset, key, value);
				return pair<iterator, OperationResult>(iterator(), t);
			}
			
			readFile(&leaf, leafOffset, 1, sizeof(leaf_node));
			OperationResult t = leaf_insert(leaf, key, value);    //第一步
			fflush(file);
			return pair<iterator, OperationResult>(iterator(), t);
		}


		OperationResult leaf_insert(leaf_node &leaf, const Key &key, const Value &value) {     //先在叶子节点插入，再向父亲节点维护
			int pos = 0;

			for (; pos < leaf.num; pos++) {     //找到data[pos-1]<=key<=data[pos]
				if (key == leaf.data[pos].first)
					return Fail;
				if (key < leaf.data[pos].first)
					break;
			}

			for (int i = leaf.num - 1; i >= pos; --i) {
				leaf.data[i + 1].first = leaf.data[i].first;
				leaf.data[i + 1].second = leaf.data[i].second;
			}

			leaf.num++;
			basic_info.tree_size++;


			leaf.data[pos].first = key;
			leaf.data[pos].second = value;

			writeFile(&basic_info, 0, 1, sizeof(basic_information));

			if (leaf.num <= L)
				writeFile(&leaf, leaf.offset, 1, sizeof(leaf_node));
			else
				leaf_split(leaf, key);

			fflush(file);

			return Success;
		}

		void internode_insert(inter_node &node, const Key &key, int newSon) {    //叶子结点分裂时调用
			int pos = 0;
			for (; pos < node.num; pos++)
				if (key < node.data[pos]) break;
			for (int i = node.num - 1; i >= pos; i--)
				node.data[i + 1] = node.data[i];
			for (int i = node.num - 1; i >= pos; i--)
				node.son[i + 1] = node.son[i];
			node.data[pos] = key;
			node.son[pos] = newSon;
			node.num++;
			if (node.num <= M) writeFile(&node, node.offset, 1, sizeof(inter_node));
			else internode_split(node);
			fflush(file);
		}

		void leaf_split(leaf_node &leaf, const Key &key) {

			leaf_node newLeaf;
			leaf_node nextLeaf;

			newLeaf.num = leaf.num / 2;
			leaf.num -= newLeaf.num;

			newLeaf.offset = basic_info.end;
			basic_info.end += sizeof(leaf_node);
			newLeaf.parent = leaf.parent;

			for (int i = 0; i < newLeaf.num; i++) {
				newLeaf.data[i].first = leaf.data[i + leaf.num].first;
				newLeaf.data[i].second = leaf.data[i + leaf.num].second;
			}

			newLeaf.next = leaf.next;
			newLeaf.prev = leaf.offset;
			leaf.next = newLeaf.offset;

			if (newLeaf.next != 0) {      //注意尾节点没有后继节点
				readFile(&nextLeaf, newLeaf.next, 1, sizeof(leaf_node));
				nextLeaf.prev = newLeaf.offset;
				writeFile(&nextLeaf, nextLeaf.offset, 1, sizeof(leaf_node));
			}                                  //完成双链表链接

			if (basic_info.tail == leaf.offset)
				basic_info.tail = newLeaf.offset; // 新节点比较大，更容易作为尾节点

			writeFile(&leaf, leaf.offset, 1, sizeof(leaf_node));
			writeFile(&newLeaf, newLeaf.offset, 1, sizeof(leaf_node));
			writeFile(&basic_info, 0, 1, sizeof(basic_information));

			inter_node parent;
			readFile(&parent, leaf.parent, 1, sizeof(inter_node));

			internode_insert(parent, newLeaf.data[0].first, newLeaf.offset);    //往父节点插新点

			fflush(file);
		}



		void internode_split(inter_node &node) {

			inter_node newNode;
			newNode.num = node.num / 2;
			node.num -= newNode.num;
			newNode.parent = node.parent;
			newNode.son_type = node.son_type;
			newNode.offset = basic_info.end;
			basic_info.end += sizeof(inter_node);

			for (int i = 0; i < newNode.num; ++i) {
				newNode.son[i] = node.son[node.num + i];
				newNode.data[i] = node.data[node.num + i];
			}
			            //=====处理儿子节点的父亲结点链接=======//   <-----有点特殊

			leaf_node leaf;
			inter_node tmp;
			for (int j = 0; j < newNode.num; ++j) {
				if (newNode.son_type) {              //这两个没什么两样 只是大小类型不同
					readFile(&leaf, newNode.son[j], 1, sizeof(leaf_node));
					leaf.parent = newNode.offset;
					writeFile(&leaf, leaf.offset, 1, sizeof(leaf_node));
				}
				else {
					readFile(&tmp, newNode.son[j], 1, sizeof(inter_node));
					tmp.parent = newNode.offset;
					writeFile(&tmp, tmp.offset, 1, sizeof(inter_node));
				}
			}

			           //=====处理儿子节点的父亲结点链接=======//


			if (node.offset == basic_info.root) {        //这是根的分裂
				inter_node newRoot;
				newRoot.parent = newRoot.son_type = 0;
				newRoot.offset = basic_info.end;
				basic_info.end += sizeof(inter_node);

				newRoot.num = 2;
				newRoot.data[0] = node.data[0];
				newRoot.data[1] = newNode.data[0];

				newRoot.son[0] = node.offset;
				newRoot.son[1] = newNode.offset;

				basic_info.root = node.parent = newNode.parent = newRoot.offset;

				writeFile(&node, node.offset, 1, sizeof(inter_node));
				writeFile(&newNode, newNode.offset, 1, sizeof(inter_node));
				writeFile(&newRoot, newRoot.offset, 1, sizeof(inter_node));
				writeFile(&basic_info, 0, 1, sizeof(basic_information));
			}
			else {
				inter_node parent;

				writeFile(&basic_info, 0, 1, sizeof(basic_information));
				writeFile(&node, node.offset, 1, sizeof(inter_node));
				writeFile(&newNode, newNode.offset, 1, sizeof(inter_node));

				readFile(&parent, node.parent, 1, sizeof(inter_node));

				internode_insert(parent, newNode.data[0], newNode.offset);    //非根分裂向上维护
			}
			fflush(file);
		}

		OperationResult min_insert(leaf_node &leaf, int leafOffset, const Key &key, const Value &value) {
			readFile(&leaf, basic_info.head, 1, sizeof(leaf_node));
			OperationResult t = leaf_insert(leaf, key, value);

			if (t == Fail) return t;

			int offset = leaf.parent;
			inter_node node;

			while (offset != 0) {     //根节点父节点偏移量0
				readFile(&node, offset, 1, sizeof(inter_node));

				node.data[0] = key;

				writeFile(&node, offset, 1, sizeof(inter_node));

				offset = node.parent;
			}
			fflush(file);
			return t;
		}

		int find_pos(const Key &key, int offset) {
			inter_node p;
			readFile(&p, offset, 1, sizeof(inter_node));
			if (p.son_type) {
				int pos = 0;
				for (; pos < p.num; pos++)
					if (key < p.data[pos]) break;
				if (pos == 0) return 0;
				return p.son[pos - 1];
			}
			else {
				int pos = 0;
				for (; pos < p.num; pos++)
					if (key < p.data[pos]) break;    // data[pos-1]<=key< data[pos]
				if (pos == 0) return 0;
				return find_pos(key, p.son[pos - 1]);
			}
		}

		Value at(const Key &key) {
			int leaf_offset = find_pos(key, basic_info.root);

			leaf_node leaf;

			readFile(&leaf, leaf_offset, 1, sizeof(leaf_node));
			for (int i = 0; i < leaf.num; i++)
				if (leaf.data[i].first == key)
					return leaf.data[i].second;
		}

		// Erase: Erase the Key-Value
		// Return Success if it is successfully erased
		// Return Fail if the key doesn't exist in the database

		OperationResult erase(const Key &key) {     //insert写到自闭
			// TODO erase function
			return Fail;  // If you can't finish erase part, just remaining here.
		}

		bool empty() const {
			return basic_info.tree_size == 0;
		}

		// Return the number of <K,V> pairs
		size_t size() const {
			return basic_info.tree_size;
		}
	};
} // namespace sjtu
