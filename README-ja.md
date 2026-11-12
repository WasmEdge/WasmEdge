<div align="right">

  [Readme in English](README.md) | [中文](README-zh.md) | [正體中文](README-zh-TW.md)

</div>

<div align="center">

![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

# [🤩 WasmEdge は、手元のデバイスで LLM を実行するための、最も簡単で最速の方法です。 🤩](https://llamaedge.com/docs/intro)

<a href="https://trendshift.io/repositories/2481" target="_blank"><img src="https://trendshift.io/api/badge/repositories/2481" alt="WasmEdge%2FWasmEdge | Trendshift" style="width: 250px; height: 55px;" width="250" height="55"/></a>

WasmEdge は、軽量・高性能・拡張可能な WebAssembly ランタイムであり、[最速の Wasm VM](https://ieeexplore.ieee.org/document/9214403) です。また、[CNCF](https://www.cncf.io/) 傘下の公式サンドボックスプロジェクトでもあります。[LlamaEdge](https://github.com/LlamaEdge/LlamaEdge) は、生成 AI モデル（例えば、[LLM](https://llamaedge.com/docs/intro)、[speech-to-text](https://llamaedge.com/docs/user-guide/speech-to-text/quick-start-whisper)、[text-to-image](https://llamaedge.com/docs/user-guide/text-to-image/quick-start-sd)、[TTS](https://github.com/LlamaEdge/whisper-api-server)）を動かすために WasmEdge 上に構築されたアプリケーションフレームワークであり、GPU も活用しながら、サーバー、一般的な PC、エッジデバイスなどの環境で実行できます。その他の [ユースケース](https://wasmedge.org/docs/start/usage/use-cases/) には、エッジクラウド上のマイクロサービス、サーバーレス SaaS API、アプリケーション拡張用の埋め込み関数、スマートコントラクト、スマートデバイスなどが含まれます。

[![build](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml?query=event%3Apush++branch%3Amaster)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml?query=event%3Apush++branch%3Amaster)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# クイックスタートガイド

🚀 WasmEdge の [インストール](https://wasmedge.org/docs/start/install) \
👷🏻‍♂️ WasmEdge の [ビルド](https://wasmedge.org/docs/category/build-wasmedge-from-source) と [開発への参加](https://wasmedge.org/docs/contribute/) \
⌨️ CLI または [Docker](https://wasmedge.org/docs/start/getting-started/quick_start_docker) から、単体で実行可能な Wasm プログラムまたは [JavaScript プログラム](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript) を [実行](https://wasmedge.org/docs/category/running-with-wasmedge) \
🤖 [LlamaEdge](https://github.com/LlamaEdge/LlamaEdge) 経由でオープンソース LLM と [チャット](https://llamaedge.com/docs/intro)  \
🔌 [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge)、[Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge)、[C](https://wasmedge.org/docs/category/c-sdk-for-embedding-wasmedge) アプリに Wasm 関数を組み込む \
🛠 [Kubernetes](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)、[データストリーミングフレームワーク](https://wasmedge.org/docs/embed/use-case/yomo)、[ブロックチェーン](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a) を使用して Wasm ランタイムを管理し、オーケストレーションする \
📚 **[公式ドキュメントをチェック](https://wasmedge.org/docs/)**

# イントロダクション

WasmEdge ランタイムは、内部で実行される WebAssembly バイトコードプログラムに対して、明確に定義された実行サンドボックスを提供します。このランタイムは、オペレーティングシステムリソース（ファイルシステム、ソケット、環境変数、プロセスなど）とメモリ空間を分離し、保護します。WasmEdge の最も重要なユースケースは、ソフトウェア製品（SaaS、ソフトウェア定義車両、エッジノード、あるいはブロックチェーンノードなど）のプラグインとして、ユーザー定義コードやコミュニティから提供されたコードを安全に実行することです。これにより、サードパーティの開発者、ベンダー、サプライヤー、コミュニティメンバーがソフトウェア製品を拡張し、カスタマイズすることが可能になります。**[詳細はこちら](https://wasmedge.org/docs/contribute/users)**

## パフォーマンス

* [A Lightweight Design for High-performance Serverless Computing](https://arxiv.org/abs/2010.07115), published on IEEE Software, Jan 2021. [https://arxiv.org/abs/2010.07115](https://arxiv.org/abs/2010.07115)
* [Performance Analysis for Arm vs. x86 CPUs in the Cloud](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/), published on infoQ.com, Jan 2021. [https://www.infoq.com/articles/arm-vs-x86-cloud-performance/](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)

## 特徴

WasmEdge は、C/C++、Rust、Swift、AssemblyScript、または Kotlin のソースコードからコンパイルされた標準的な WebAssembly バイトコードプログラムを実行できます。サードパーティの ES6、CJS、NPM モジュールを含む [JavaScript](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript) を、安全、高速、軽量、ポータブル、コンテナ化されたサンドボックスで実行します。また、これらの言語を組み合わせた開発（例えば、[JavaScript API を実装するために Rust を使用する](https://wasmedge.org/docs/develop/javascript/rust)）、[Fetch](https://wasmedge.org/docs/develop/javascript/networking#fetch-client) API、エッジサーバー上の [サーバーサイドレンダリング（SSR）](https://wasmedge.org/docs/develop/javascript/ssr) もサポートしています。

WasmEdge は、[すべての標準的な WebAssembly 機能と多くの提案段階の拡張機能](https://wasmedge.org/docs/start/wasmedge/extensions/proposals)をサポートしています。また、クラウドネイティブやエッジコンピューティングの用途に合わせた拡張機能も多数サポートしています（[WasmEdge ネットワークソケット](https://wasmedge.org/docs/category/socket-networking)、[Postgres および MySQL ベースのデータベースドライバ](https://wasmedge.org/docs/category/database-drivers)、[WasmEdge AI 拡張機能](https://wasmedge.org/docs/category/ai-inference)など）。

**WasmEdge の [テクニカルハイライト](https://wasmedge.org/docs/start/wasmedge/features) についてはこちらをご覧ください。**

## インテグレーションと管理

WasmEdge とそれに含まれる Wasm プログラムは、新規プロセスとして [CLI](https://wasmedge.org/docs/category/running-with-wasmedge) から起動することも、既存プロセスから起動することもできます。既存のプロセス（例えば、実行中の [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge) や [Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge) プログラム）から呼び出した場合、WasmEdge は別プロセスとしてではなく、既存プロセス内で関数として実行されます。現在のところ、WasmEdge はまだスレッドセーフではありません。独自のアプリケーションやクラウドネイティブフレームワークで WasmEdge を使用するには、以下のガイドを参照してください。

* [WasmEdge をホストアプリケーションに組み込む](https://wasmedge.org/docs/embed/overview)
* [コンテナツールを使用した WasmEdge インスタンスのオーケストレーションと管理](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)
* [WasmEdge アプリを Dapr マイクロサービスとして実行する](https://wasmedge.org/docs/develop/rust/dapr)

# コミュニティ

## 貢献

コミュニティからの貢献を歓迎します。はじめに、以下をご確認ください。
- 始め方については [コントリビューティングガイド](./docs/CONTRIBUTING.md)
- プロジェクトの意思決定プロセスについては [ガバナンス文書](./docs/GOVERNANCE.md)
- コミュニティの行動規範については [行動規範](./docs/CODE_OF_CONDUCT.md)

メンテナーを目指したい場合は、[コントリビューターラダー](./docs/CONTRIBUTOR_LADDER.md) をご覧ください。

## ロードマップ

[プロジェクトロードマップ](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md) で、WasmEdge の今後の機能と計画をご確認ください。

## コンタクト

ご質問がある場合は、関連プロジェクトの GitHub issue を作成するか、以下のチャンネルにお気軽にご参加ください:

* メーリングリスト: [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/) にメールを送信
* Discord: [WasmEdge Discord サーバー](https://discord.gg/h4KDyB8XTt) に参加
* Slack: [CNCF Slack](https://slack.cncf.io/) の #WasmEdge チャンネルに参加
* X (旧 Twitter): [X](https://x.com/realwasmedge) で @realwasmedge をフォロー

## 採用事例

WasmEdge を利用している組織やプロジェクトについては、[採用事例の一覧](https://wasmedge.org/docs/contribute/users/) をご覧ください。

## コミュニティミーティング

月に一度、コミュニティミーティングを開催し、新機能の紹介や新しいユースケースのデモ、質疑応答などを行います。どなたでもお気軽にご参加ください！

開催日時: 毎月第 1 火曜日 午後 11 時（香港時間）/ 午前 7 時（PST、太平洋標準時）。

[パブリックミーティングのアジェンダ／ノート](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit#) | [Zoom リンク](https://us06web.zoom.us/j/82221747919?pwd=3MORhaxDk15rACk7mNDvyz9KtaEbWy.1)

# ライセンス

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
