# 参考になりそうなサイト

### アルゴリズム全体について
- [強化学習とは？機械学習・ディープラーニングとの違いを解説](https://www.sedesign.co.jp/ai-blog/what-is-reinforcement-learning)
- [世界四連覇AIエンジニアがゼロから教えるゲーム木探索入門](https://qiita.com/thun-c/items/058743a25c37c87b8aa4)
- [ゼロからDeepまで学ぶ強化学習](https://qiita.com/icoxfog417/items/242439ecd1a477ece312)
- [自作中のシンプルな分散強化学習フレームワークの紹介](https://qiita.com/pocokhc/items/a2f1ba993c79fdbd4b4d)
- [強化学習入門](https://www.brainpad.co.jp/doors/contents/01_tech_2017-02-24-121500/)
- [強化学習入門 Part2](https://www.brainpad.co.jp/doors/contents/01_tech_2017-09-08-140000/)
- [強化学習入門 Part3](https://www.brainpad.co.jp/doors/contents/01_tech_2018-04-05-163000/)

### 特定のアルゴリズムの実装/考察記事
- [NSLによるBlokus Duo Multi Game AIシステムの実装](https://www.u-tokai.ac.jp/uploads/sites/12/2021/03/PP9-16.pdf)
- [『強い将棋ソフトの創りかた』ではじめての将棋AI開発](https://qiita.com/Shu2424/items/7c4e042ea5892569171e)
- [Blokus Duo Game on FPGA ](https://www.cs.ucr.edu/~ajaha004/files/Blokus.pdf)
- [DQNを用いたブロックスのゲームAI](https://www.gifu-nct.ac.jp/elec/deguchi/sotsuron/mizuno.pdf)
### コラム
- [AlphaGo概要解説](https://qiita.com/gifucom17/items/3096ac60522f8b815a32)
	（感想）この短期間で教師あり学習をやるのは難しそう？
- 
# 参考になりそうな本
- ゲームで学ぶ探索アルゴリズム実践入門[https://gihyo.jp/book/2023/978-4-297-13360-3]
- 強い将棋ソフトの創りかた[http://book.mynavi.jp/ec/products/detail/id=126887]

# 個人の雑感/考察
- ゲーム木複雑性からブロックスの探索方法を考える
-- 手数は最大42手
-- 取りうる手は（配置候補場所）×（21コマ）×（4方向）　※計算しやすいので一旦まとめて100とする
-- おおよそ100の40乗→計算量膨大すぎて工夫は必須→どうしよう…