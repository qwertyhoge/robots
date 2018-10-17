# robots
Msweeperと同じ授業で作成したもの。それと同様に、設計に気を遣うよう指示を受けたため、ドキュメントを残すようにしてある。言語はC。

# 使った技術

- 構造体
- 構造体ポインタによる動的確保(再帰的ではない)
- 外部ファイルにある関数の読み込み(extern)
- 二つのデータ構造の使い分けと同期
- ファイル操作と時間取得

# 反省
処理があまり効率的でないらしい。(教員談)  
進捗のメモはいくらか詳細だが、プログラムに付加されているコメントがあまり親切でなく、ざっと読んでもどんな処理をしているのか今一つ頭に入ってこない。しかし、変数名はよく考えられており、少し目を凝らして読めば大方の内容は理解できるようになっている。  
バグ探しは丁寧で、細かい条件で発生するものでも原因を特定し、改善することに成功している。とはいえ、無理矢理な修正が多いようで、確かに効率的でない処理をしている可能性は高く見える。

# 自己評価
効率があまり高くなさそうとはいえ、バグがよく発生する複雑な判定も、デバッグをきちんと行って対象できることを証明している。  
可読性もいくらか確保してあり、他のプログラマでも読み込めば大部分は理解できそうなコードになっている。  
アルゴリズムに起因する拡張性の低さを踏まえると、実用レベルにあと一歩というところだろうか。