    
/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
* @LastEditTime: 2025-08-29
*/
/***************************************************************************************************************************************************/
#include <Windows.h>
#include "Common/RBTree.h"
#pragma comment(lib,"libSdk.lib")
int main()
{
	RedBlackTree<int> rbTree;

	rbTree.insert(7);
	rbTree.insert(3);
	rbTree.insert(18);
	rbTree.insert(10);
	rbTree.insert(22);
	rbTree.insert(8);
	rbTree.insert(11);
	rbTree.insert(26);


    cout << "Red-Black Tree after insertion:" << endl;
    rbTree.printTree();

    cout << "\nDeleting 18..." << endl;
    rbTree.remove(18);
    rbTree.printTree();

    cout << "\nSearching for 11: ";
    auto result = rbTree.search(11);
    if (result) {
        cout << "Found (" << result->data << ")" << endl;
    }
    else {
        cout << "Not found" << endl;
    }	
}