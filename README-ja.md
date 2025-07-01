<div align="right">

  [中文](README-zh.md) | [正體中文](README-zh-TW.md) | [日本語で読む](README-ja.md)

</div>

<div align="center">
  
![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

# [🤩 WasmEdgeは、あなた自身のデバイスでLLMを実行する最も簡単で最速の方法です。🤩](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge)

<a href="https://trendshift.io/repositories/2481" target="_blank"><img src="https://trendshift.io/api/badge/repositories/2481" alt="WasmEdge%2FWasmEdge | Trendshift" style="width: 250px; height: 55px;" width="250" height="55"/></a>

WasmEdgeは、軽量で高性能、そして拡張可能なWebAssemblyランタイムです。これは[最速のWasm VM](https://ieeexplore.ieee.org/document/9214403)です。WasmEdgeは、[CNCF](https://www.cncf.io/)によってホストされている公式のサンドボックスプロジェクトです。[LlamaEdge](https://github.com/LlamaEdge/LlamaEdge)は、WasmEdge上に構築されたアプリケーションフレームワークであり、サーバー、パーソナルコンピュータ、およびエッジデバイスのGPU全体でGenAIモデル（例：[LLM](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge)、[音声からテキストへ](https://llamaedge.com/docs/user-guide/speech-to-text/quick-start-whisper)、[テキストから画像へ](https://llamaedge.com/docs/user-guide/text-to-image/quick-start-sd)、および[TTS](https://github.com/LlamaEdge/whisper-api-server)）を実行します。追加の[ユースケース](https://wasmedge.org/docs/start/usage/use-cases/)には、エッジクラウド上のマイクロサービス、サーバーレスSaaS API、組み込み関数、スマートコントラクト、およびスマートデバイスが含まれます。

[![build](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml?query=event%3Apush++branch%3Amaster)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml?query=event%3Apush++branch%3Amaster)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# クイックスタートガイド

🚀 WasmEdgeを[インストール](https://wasmedge.org/docs/start/install)する \
👷🏻‍♂️ WasmEdgeを[ビルド](https://wasmedge.org/docs/category/build-wasmedge-from-source)し、[貢献](https://wasmedge.org/docs/contribute/)する \
⌨️ CLIまたは[Docker](https://wasmedge.org/docs/start/getting-started/quick_start_docker)からスタンドアロンのWasmプログラムまたは[JavaScriptプログラム](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript)を[実行](https://wasmedge.org/docs/category/running-with-wasmedge)する \
🤖 [LlamaEdge](https://github.com/LlamaEdge/LlamaEdge)を介してオープンソースのLLMと[チャット](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge)する \
🔌 [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge)、[Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge)、または[C](https://wasmedge.org/docs/category/c-sdk-for-embedding-wasmedge)アプリにWasm関数を埋め込む \
🛠 [Kubernetes](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)、[データストリーミングフレームワーク](https://wasmedge.org/docs/embed/use-case/yomo)、および[ブロックチェーン](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a)を使用してWasmランタイムを管理およびオーケストレーションする \
📚 **[公式ドキュメントをチェックしてください](https://wasmedge.org/docs/)**

# はじめに

WasmEdgeランタイムは、それに含まれるWebAssemblyバイトコードプログラムに対して、明確に定義された実行サンドボックスを提供します。ランタイムは、オペレーティングシステムリソース（ファイルシステム、ソケット、環境変数、プロセスなど）およびメモリ空間の分離と保護を提供します。WasmEdgeの最も重要なユースケースは、ソフトウェア製品（SaaS、ソフトウェア定義車両、エッジノード、さらにはブロックチェーンノードなど）のプラグインとして、ユーザー定義またはコミュニティ提供のコードを安全に実行することです。これにより、サードパーティの開発者、ベンダー、サプライヤー、およびコミュニティメンバーがソフトウェア製品を拡張およびカスタマイズできます。**[詳細はこちら](https://wasmedge.org/docs/contribute/users)**

## パフォーマンス

* [高性能サーバーレスコンピューティングのための軽量設計](https://arxiv.org/abs/2010.07115)、IEEE Softwareに掲載、2021年1月。[https://arxiv.org/abs/2010.07115](https://arxiv.org/abs/2010.07115)
* [クラウドにおけるArm対x86 CPUのパフォーマンス分析](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)、infoQ.comに掲載、2021年1月。[https://www.infoq.com/articles/arm-vs-x86-cloud-performance/](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)
* [WasmEdgeはSuborbital Reactrテストスイートで最速のWebAssemblyランタイムです](https://blog.suborbital.dev/suborbital-wasmedge)、2021年12月

## 特徴

WasmEdgeは、C/C++、Rust、Swift、AssemblyScript、またはKotlinのソースコードからコンパイルされた標準のWebAssemblyバイトコードプログラムを実行できます。安全で高速、軽量、ポータブル、そしてコンテナ化されたサンドボックスで、サードパーティのES6、CJS、NPMモジュールを含む[JavaScriptを実行](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript)します。また、これらの言語の混合（例：[JavaScript APIを実装するためにRustを使用する](https://wasmedge.org/docs/develop/javascript/rust)）、[Fetch API](https://wasmedge.org/docs/develop/javascript/networking#fetch-client)、およびエッジサーバーでの[サーバーサイドレンダリング（SSR）](https://wasmedge.org/docs/develop/javascript/ssr)機能もサポートしています。

WasmEdgeは、[すべての標準的なWebAssembly機能と多くの提案されている拡張機能](https://wasmedge.org/docs/start/wasmedge/extensions/proposals)をサポートしています。また、クラウドネイティブおよびエッジコンピューティング用途に合わせた多くの拡張機能（例：[WasmEdgeネットワークソケット](https://wasmedge.org/docs/category/socket-networking)、[PostgresおよびMySQLベースのデータベースドライバ](https://wasmedge.org/docs/category/database-drivers)、および[WasmEdge AI拡張機能](https://wasmedge.org/docs/category/ai-inference)）もサポートしています。

**[WasmEdgeの技術的なハイライト](https://wasmedge.org/docs/start/wasmedge/features)について詳しくはこちら。**

## 統合と管理

WasmEdgeおよびそれに含まれるwasmプログラムは、新しいプロセスとして[CLI](https://wasmedge.org/docs/category/running-with-wasmedge)から、または既存のプロセスから開始できます。既存のプロセス（実行中の[Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge)または[Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge)プログラムなど）から開始した場合、WasmEdgeはプロセス内で関数として単純に実行されます。現在、WasmEdgeはまだスレッドセーフではありません。独自のアプリケーションまたはクラウドネイティブフレームワークでWasmEdgeを使用するには、以下のガイドを参照してください。

* [ホストアプリケーションにWasmEdgeを埋め込む](https://wasmedge.org/docs/embed/overview)
* [コンテナツールを使用してWasmEdgeインスタンスをオーケストレーションおよび管理する](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)
* [WasmEdgeアプリをDaprマイクロサービスとして実行する](https://wasmedge.org/docs/develop/rust/dapr)

# コミュニティ

## 貢献

コミュニティからの貢献を歓迎します！以下をご確認ください：
- [貢献ガイド](./docs/CONTRIBUTING.md)で開始方法を確認
- [ガバナンスドキュメント](./docs/GOVERNANCE.md)でプロジェクトの意思決定プロセスを確認
- [行動規範](./docs/CODE_OF_CONDUCT.md)でコミュニティの基準を確認

メンテナーになりたいですか？[貢献者ラダー](./CONTRIBUTION_LADDER.md)をご覧ください。

## ロードマップ

[プロジェクトロードマップ](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md)をチェックして、WasmEdgeの今後の機能と計画を確認してください。

## 連絡先

ご不明な点がございましたら、関連プロジェクトでGitHubの問題を開くか、次のチャネルにご参加ください。

* メーリングリスト：[WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)にメールを送信
* Discord：[WasmEdge Discordサーバー](https://discord.gg/h4KDyB8XTt)に参加してください！
* Slack：[CNCF Slack](https://slack.cncf.io/)の#WasmEdgeチャンネルに参加
* X（旧Twitter）：[X](https://x.com/realwasmedge)で@realwasmedgeをフォロー

## 採用者

[採用者リスト](https://wasmedge.org/docs/contribute/users/)をチェックして、プロジェクトでWasmEdgeを使用しているユーザーを確認してください。

## コミュニティミーティング

私たちは毎月コミュニティミーティングを開催し、新機能の紹介、新しいユースケースのデモ、Q&Aセッションを行っています。どなたでも大歓迎です！

時間：毎月第1火曜日、香港時間午後11時/太平洋標準時午前7時。

[公開会議の議題/議事録](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit#) | [Zoomリンク](https://us06web.zoom.us/j/82221747919?pwd=3MORhaxDk15rACk7mNDvyz9KtaEbWy.1)

# ライセンス

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
