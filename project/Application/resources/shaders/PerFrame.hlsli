
struct PerFrame
{
	// ゲームを起動してからの時間。
	// 今回使うわけではないが、PerFrameにはこのような
	// パラメータを入れておくと良い
	float32_t time;
	// 1フレームの経過時間
	float32_t deltaTime;
};