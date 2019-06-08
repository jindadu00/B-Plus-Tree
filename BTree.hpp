
#include "utility.hpp"
#include <functional>
#include <cstddef>
#include <map>
#include <fstream>
#include <stdio.h>

const int size_M = 1000;
const int size_L = 200;

using namespace std;

namespace sjtu {
	typedef size_t position;

	template <class Key, class Value, class Compare = std::less<Key> >
	class BTree {
	public:
		typedef pair<Key, Value> value_type;
		struct inter_node {
			int current_size;
			position cur_offset;
			position parent, leftgood, rightgood;
			bool rank;//判断孩子节点是否是叶子0为孩子是叶子1为孩子是节点
			position child[size_M + 1];
			Key keylist[size_M + 1];
			inter_node() {
				cur_offset = 0;
				parent = 0;
				leftgood = 0;
				rightgood = 0;
				for (int i = 0; i < current_size; ++i) child[i] = 0;
			}
		};
		struct leaf_node {
			position cur_offset;
			position parent, leftgood, rightgood;
			int current_size;
			value_type data[size_L + 1];
			leaf_node() {
				cur_offset = 0;
				parent = 0;
				leftgood = 0;
				rightgood = 0;
			}
		};
		struct basic_information {
			position head;
			position root;
			position tail;
			position end;

			int Treesize;

			basic_information() {
				head = 0;
				root = 0;
				tail = 0;
				Treesize = 0;
			}
		};
		FILE *file;

		basic_information inf;
		// Your private members go here

		void writeNode(void *thing, size_t offset, size_t size, size_t num) {
			if (fseek(file, offset, 0)) throw "error";
			fwrite(thing, size, num, file);
			fflush(file);
		}
		// Default Constructor and Copy Constructor
		BTree() {
			file = fopen("123.txt", "w");
			fclose(file);
			file = fopen("123.txt", "rb+");

			inter_node Root;
			leaf_node Head;
			inf.Treesize = 0;
			Root.cur_offset = inf.root = sizeof(basic_information);
			inf.head = Head.cur_offset = sizeof(basic_information) + sizeof(inter_node);
			inf.tail = sizeof(basic_information) + sizeof(inter_node) + sizeof(leaf_node);
			inf.end = inf.tail;
			Root.parent = 0;
			Root.current_size = 1; Head.current_size = 0;
			Root.child[0] = Head.cur_offset;
			Head.parent = Root.cur_offset;
			Root.rank = 0;//孩子是叶子
			writeNode(&inf, 0, sizeof(basic_information), 1);
			writeNode(&Root, Root.cur_offset, sizeof(inter_node), 1);
			writeNode(&Head, Head.cur_offset, sizeof(leaf_node), 1);
			// Todo Default
		}

		~BTree() {
			fclose(file);
		}
		// Insert: Insert certain Key-Value into the database
		// Return a pair, the first of the pair is the iterator point to the new
		// element, the second of the pair is Success if it is successfully inserted

		OperationResult insertdata(const Key key, const Value value) {
			inter_node tmp;
			inf.Treesize++;
			if (fseek(file, inf.root, 0)) throw "error";

			fread(&tmp, sizeof(inter_node), 1, file);
			while (tmp.rank) {
				int i;
				for (i = 0; i < tmp.current_size; ++i) if (key < tmp.keylist[i]) break;
				if (fseek(file, tmp.child[i], 0)) throw "error";

				fread(&tmp, sizeof(inter_node), 1, file);
			}
			int i;
			for (i = 0; i < tmp.current_size; ++i) if (key < tmp.keylist[i]) break;
			int insert_target = i;
			leaf_node tmpdata;
			if (fseek(file, tmp.child[i], 0))
				throw "error";

			fread(&tmpdata, sizeof(leaf_node), 1, file);
			for (i = 0; i < tmpdata.current_size; ++i) {
				if (key == tmpdata.data[i].first) return Fail;
				if (key < tmpdata.data[i].first) break;
			}
			for (int j = tmpdata.current_size; j > i + 1; --j) {
				tmpdata.data[j].first = tmpdata.data[j - 1].first;
				tmpdata.data[j].second = tmpdata.data[j - 1].second;
			}

			tmpdata.data[i].first = key;
			tmpdata.data[i].second = value;
			tmpdata.current_size++;

			writeNode(&tmpdata, tmpdata.cur_offset, sizeof(leaf_node), 1);
			if (tmpdata.current_size > size_L) {
				splitleaf(tmpdata, tmp, insert_target);
				if (fseek(file, tmpdata.parent, 0))  throw "error";
				fread(&tmp, sizeof(inter_node), 1, file);
				if (tmp.current_size > size_M) {
					while (tmp.parent != 0) {
						splitnode(tmp);
						if (fseek(file, tmp.parent, 0))  throw "error";
						fread(&tmp, sizeof(inter_node), 1, file);
						if (tmp.current_size <= size_M) break;
					}
					if (tmp.current_size > size_M) splitRoot(tmp);
				}
			}
			return Success;
		}
		void splitnode(inter_node &tmpnode) {
			inter_node newnode;

			int num = tmpnode.current_size;
			newnode.rank = tmpnode.rank;
			newnode.current_size = tmpnode.current_size - (tmpnode.current_size >> 1);
			tmpnode.current_size = tmpnode.current_size >> 1;
			newnode.parent = tmpnode.parent;
			for (int i = tmpnode.current_size; i < num; ++i) {
				newnode.keylist[i - tmpnode.current_size] = tmpnode.keylist[i];
				newnode.child[i - tmpnode.current_size] = tmpnode.child[i];
			}

			newnode.cur_offset = inf.end;
			inf.end += sizeof(inter_node);
			writeNode(&tmpnode, tmpnode.cur_offset, sizeof(inter_node), 1);
			writeNode(&newnode, newnode.cur_offset, sizeof(inter_node), 1);
			inter_node Treeson;
			leaf_node datason;
			/*
			为儿子们赋值父亲节点地址
			*/
			for (int i = 0; i < newnode.current_size; ++i) {
				if (fseek(file, newnode.child[i], 0)) throw "error";
				if (tmpnode.rank) {
					fread(&Treeson, sizeof(inter_node), 1, file);

					Treeson.parent = newnode.cur_offset;
					writeNode(&Treeson, newnode.child[i], sizeof(inter_node), 1);
				}
				else {
					fread(&datason, sizeof(leaf_node), 1, file);

					datason.parent = newnode.cur_offset;
					writeNode(&datason, newnode.child[i], sizeof(leaf_node), 1);
				}
			}
			inter_node par;
			if (fseek(file, tmpnode.parent, 0)) throw "error";
			fread(&par, sizeof(inter_node), 1, file);
			int posi = findNode(par, tmpnode.keylist[0]);
			for (int i = par.current_size; i > posi + 1; --i) {
				par.keylist[i] = par.keylist[i - 1];
				par.child[i] = par.child[i - 1];
			}
			par.keylist[posi + 1] = newnode.keylist[0];
			par.child[posi + 1] = newnode.cur_offset;
			writeNode(&par, par.cur_offset, sizeof(inter_node), 1);
		}
		void splitRoot(inter_node &root) {
			inter_node newRoot;
			inter_node newnode;
			int num;
			newRoot.rank = 1;
			newRoot.parent = 0;
			newRoot.cur_offset = inf.root = inf.end;
			//writeNode(&newRoot, inf.end, sizeof(inter_node), file);
			inf.end += sizeof(inter_node);
			newnode.cur_offset = inf.end;
			inf.end += sizeof(inter_node);
			num = root.current_size;
			newnode.rank = root.rank;
			newnode.current_size = root.current_size - (root.current_size >> 1);
			root.current_size = root.current_size >> 1;
			for (int i = root.current_size; i < num; ++i) {
				newnode.keylist[i - root.current_size] = root.keylist[i];
				newnode.child[i - root.current_size] = root.child[i];
			}
			newnode.parent = root.parent = inf.root;
			newRoot.keylist[0] = root.keylist[0];
			newRoot.child[0] = root.cur_offset;
			newRoot.keylist[1] = newnode.keylist[0];
			newRoot.child[1] = newnode.cur_offset;
			writeNode(&newRoot, newRoot.cur_offset, sizeof(inter_node), 1);
			writeNode(&newnode, newnode.cur_offset, sizeof(inter_node), 1);

		}
		void splitleaf(leaf_node &tmpdata, inter_node &tmp, const int sonposi) {
			leaf_node newleaf;
			Key newkey;
			int num = tmpdata.current_size;
			newleaf.current_size = tmpdata.current_size - (tmpdata.current_size >> 1);
			tmpdata.current_size = tmpdata.current_size >> 1;
			for (int i = tmpdata.current_size; i < num; ++i) {
				tmpdata.data[i].first = tmpdata.data[i - 1].first;
				tmpdata.data[i].second = tmpdata.data[i - 1].second;
			}
			newkey = newleaf.data[0].first;
			newleaf.parent = tmpdata.parent;
			newleaf.cur_offset = inf.end;
			writeNode(&tmpdata, tmpdata.cur_offset, sizeof(leaf_node), 1);
			writeNode(&newleaf, inf.end, sizeof(leaf_node), 1);
			for (int i = tmp.current_size - 1; i > sonposi; --i) {
				tmp.child[i + 1] = tmp.child[i];
				tmp.keylist[i + 1] = tmp.keylist[i];
			}
			tmp.child[sonposi + 1] = newleaf.cur_offset;
			tmp.keylist[sonposi + 1] = newkey;
			tmp.current_size++;
			writeNode(&tmp, tmp.cur_offset, sizeof(inter_node), 1);
			inf.end += sizeof(leaf_node);
		}
		OperationResult insert(const Key& key, const Value& value) {
			return insertdata(key, value);
		}
		int findNode(inter_node &par, Key ke) {
			int i;
			for (i = 0; i < par.current_size; ++i) if (ke == par.keylist[i]) return i;
		}
		// Erase: Erase the Key-Value
		// Return Success if it is successfully erased
		// Return Fail if the key doesn't exist in the database
		OperationResult erase(const Key& key) {
			// TODO erase function
			return Fail;  // If you can't finish erase part, just remaining here.
		}

		// Check whether this BTree is empty
		bool empty() const { return inf.Treesize == 0; }
		// Return the number of <K,V> pairs
		size_t size() const { return inf.Treesize; }
		// Clear the BTree
		void clear() {
			file = fopen("123.txt", "w");
			fclose(file);
			file = fopen("123.txt", "rb+");

			inter_node Root;
			leaf_node Head;
			inf.Treesize = 0;
			Root.cur_offset = inf.root = sizeof(basic_information);
			inf.head = Head.cur_offset = sizeof(basic_information) + sizeof(inter_node);
			inf.tail = sizeof(basic_information) + sizeof(inter_node) + sizeof(leaf_node);
			inf.end = inf.tail;
			Root.parent = 0;
			Root.current_size = 1; Head.current_size = 0;
			Root.child[0] = Head.cur_offset;
			Head.parent = Root.cur_offset;
			Root.rank = 0;//孩子是叶子
			writeNode(&inf, 0, sizeof(basic_information), 1);
			writeNode(&Root, Root.cur_offset, sizeof(inter_node), 1);
			writeNode(&Head, Head.cur_offset, sizeof(leaf_node), 1);
		}
		// Return the value refer to the Key(key)
		Value at(const Key& key) {
			inter_node tmp;

			if (fseek(file, inf.root, 0)) {

				throw "error";
			}
			fread(&tmp, sizeof(inter_node), 1, file);
			while (tmp.rank) {
				int i;
				for (i = 0; i < tmp.current_size; ++i) if (key < tmp.keylist[i]) break;
				if (fseek(file, tmp.child[i], 0))

					throw "error";

				fread(&tmp, sizeof(inter_node), 1, file);
			}
			int i;
			for (i = 0; i < tmp.current_size; ++i) if (key == tmp.keylist[i]) break;
			return tmp.keylist[i].second;
		}
		/**
		 * Returns the number of elements with key
		 *   that compares equivalent to the specified argument,
		 * The default method of check the equivalence is !(a < b || b > a)
		 */
		size_t count(const Key& key) const {}
		/**
		 * Finds an element with key equivalent to key.
		 * key value of the element to search for.
		 * Iterator to an element with key equivalent to key.
		 *   If no such element is found, past-the-end (see end()) iterator is
		 * returned.
		 */
	};
}  // namespace sjtu
