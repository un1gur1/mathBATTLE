#define NOMINMAX
#include "Player.h"
#include <DxLib.h>
#include <cmath>
#include <string>
#include <unordered_map> 
#include "../../../Shader/CrystalOrbShader.h" 

namespace App {

    static int g_psPlayerCrystalHandle = -1;
    static int g_cbPlayerCrystalHandle = -1;
    static int g_playerFontHandle = -1;
    static int g_badgeFontHandle = -1;

    Player::Player(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks)
        : UnitBase("Player", startGrid, startScreen, number, stocks, maxStocks)
    {
        m_color = GetColor(255, 120, 0);

        if (g_psPlayerCrystalHandle == -1) {
            g_psPlayerCrystalHandle = LoadPixelShaderFromMem(g_ps_CrystalOrb, sizeof(g_ps_CrystalOrb));
            g_cbPlayerCrystalHandle = CreateShaderConstantBuffer(sizeof(float) * 8);
            g_playerFontHandle = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 40, 2, DX_FONTTYPE_ANTIALIASING);
            g_badgeFontHandle = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 26, 2, DX_FONTTYPE_ANTIALIASING);
        }
    }

    void DrawHexagonAA(float cx, float cy, float radius, unsigned int color, bool fill, float thickness = 1.0f, float rotAngle = 0.0f) {

        //六分割したときの角度をそれぞれ配列に格納している
        float angleOffsets[6] = { 0.0f, 60.0f, 120.0f, 180.0f, 240.0f, 300.0f };
        //デグリー(度数法)をラジアン(弧度法)に変更している
        float rad = 3.14159265f / 180.0f;

        //判定処理速度を上げるために判定式の外でラジアンの計算をまとめてせずに先に判定をしてその中でラジアンの計算をしている
		// これによって毎度fillの判定をする必要がなくなり、処理速度が上がる
        
		//引数のfillがtrueだったら塗りつぶす処理、falseだったら枠線だけを描画する処理
        if (fill) {
            //一つ目の点から二つ目の点と中心点を結んで三角形をを描画する処理を繰り返すことで六角形を描画している
            for (int i = 0; i < 6; ++i) {
				//一つ目の点の初期座標をラジアンで計算している
                float a1 = angleOffsets[i] * rad + rotAngle;
                //次の点の初期座標をラジアンで計算している
                float a2 = angleOffsets[(i + 1) % 6] * rad + rotAngle;
                //一つ目の引数は中心点の座標xyで、二つ目の引数は一つ目の点の座標xy,三つめは次の点の座標xy、であとはいろと塗りつぶすかどうかの判定をして描画している
                DrawTriangleAA(cx, cy, cx + cos(a1) * radius, cy + sin(a1) * radius, cx + cos(a2) * radius, cy + sin(a2) * radius, color, TRUE);
            }
        }
		//引数のfillがfalseだったら枠線だけを描画する処理
        else {
            //とりあえず六回回している
            for (int i = 0; i < 6; ++i) {
                //ここは枠線だけだから一つ目と二つ目の点をラジアンで計算している
                float a1 = angleOffsets[i] * rad + rotAngle;
                float a2 = angleOffsets[(i + 1) % 6] * rad + rotAngle;
                //点と点を結んでいる。
                DrawLineAA(cx + cos(a1) * radius, cy + sin(a1) * radius, cx + cos(a2) * radius, cy + sin(a2) * radius, color, thickness);
            }
        }
    }

    void Player::DrawUnitGraphic() {
        //double型でtimeを定義して、GetNowCount()は現在の時間をミリ単位で返す関数なので、1000でわって秒に変換している
        double time = GetNowCount() / 1000.0;
        //ここで揺れの幅をsin関数で計算している。sin関数は-1から1の間でへんかするので三倍の速さで動かしてその揺れの幅を四倍にしている
        float bobbing = (float)(sin(time * 3.0) * 4.0);
		//描画位置をメンバ変数で定義しているm_screenPosからxとyを取得している
        float x = m_screenPos.x;
        float y = m_screenPos.y;
		//縦に揺れるようにbobbingを足している
        float unitY = y + bobbing;

        // 1. シャドウ
        //アルファブレンドモードで透明度を設定していて、bobbingに対応して浮遊時の遠近感を影の大きさで表現している
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(80 - (bobbing + 4.0f) * 5.0f));
        //物体の中心から下にずらして陰にしていて、bobbingに対応して縦横のサイズが変わるようにしている
        DrawOvalAA(x, y + 32.0f, 24.0f - bobbing / 2.0f, 8.0f - bobbing / 4.0f, 64, GetColor(0, 50, 100), TRUE);
        //元に戻す
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. アウター・ヘックスシールド
		//サインを利用して点滅しているようにしている。サインはー１から1の間で変化するのでー０．５から０．５の間で変化するようにして、０．５を足して０から１の間で変化するようにしている
        float pulse = (float)(sin(time * 6.0) * 0.5 + 0.5);
        //時間経過で1.2倍で角度でいうと0.12秒で１回転するようにしている
        float hexRot = (float)time * 1.2f;

        //アルファブレンドで透明度をパルスに対応させて150から200の間で変化するようにしている
        SetDrawBlendMode(DX_BLENDMODE_ADD, 150 + (int)(pulse * 50));
		//外側の六角形を描画していて、回転させている
        DrawHexagonAA(x, unitY, 34.0f, m_color, FALSE, 3.0f, hexRot);
        DrawHexagonAA(x, unitY, 30.0f, m_color, FALSE, 1.0f, -hexRot * 0.8f);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 3. インナーコア
        //各ハンドルがあったら
        if (g_psPlayerCrystalHandle != -1 && g_cbPlayerCrystalHandle != -1) {
			//ピクセルシェーダーをセットして、アルファブレンドモードを設定している
            SetUsePixelShader(g_psPlayerCrystalHandle);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
			//コンスタントバッファ(定数バッファ)を色の値を設定して更新している。ここでは赤色の値を1.0、緑色の値を0.47、青色の値を0.0に設定している
            float r = 1.0f, g = 0.47f, b = 0.0f;
			//コンスタントバッファのアドレスを取得して、値を設定している。ここでは時間、赤色、緑色、青色の値を設定している
            float* cb = (float*)GetBufferShaderConstantBuffer(g_cbPlayerCrystalHandle);
			//配列を利用してバッファの引数みたいな感じで設定している。
            //cb[0]に時間、cb[1]に赤色、cb[2]に緑色、cb[3]に青色の値を設定している。cb[4]からcb[7]は0.0fで初期化している
            cb[0] = (float)time; cb[1] = r; cb[2] = g; cb[3] = b;
            cb[4] = 0.0f; cb[5] = 0.0f; cb[6] = 0.0f; cb[7] = 0.0f;
            //コンスタントバッファを更新
            UpdateShaderConstantBuffer(g_cbPlayerCrystalHandle);
			//ピクセルシェーダーにコンスタントバッファをセットしている。0はスロット番号で、ここでは0番目のスロットにセットしている
            SetShaderConstantBuffer(g_cbPlayerCrystalHandle, DX_SHADERTYPE_PIXEL, 0);

			//プレイヤーに対応する六角形の大きさを設定して、頂点情報を設定している
            float size = 28.0f;
            //頂点データ配列を用意
            VERTEX2DSHADER v[6];
			//各頂点の初期化を行っている。位置、RHW、拡散色、スペキュラ色を設定している
            for (int i = 0; i < 6; ++i) {
				//今回は2D描画なので、RHWは1.0fに設定している。拡散色は白色、スペキュラ色は黒色に設定している
				//RHWは透視投影を行う際に使用される値で、1.0fに設定することで透視投影を無効化している
				//ディフューズは物体の基本的な色を表現するための値で、今回は白色に設定しているので物体の色はそのまま表示される
				//スペキュラーは光沢の反射を表現するための値で、今回は黒色に設定しているので反射光はないことになる
                v[i].pos = VGet(0, 0, 0); v[i].rhw = 1.0f;
				//GetColorのU8はunsigined char型の意味で、0~255の範囲で色を表現することができる。
                v[i].dif = GetColorU8(255, 255, 255, 255); v[i].spc = GetColorU8(0, 0, 0, 0);
            }
			//6つの頂点の位置とテクスチャ座標を設定している。ここでは、四角形の頂点を設定している
            //左上
            v[0].pos.x = x - size; v[0].pos.y = unitY - size; v[0].u = 0.0f; v[0].v = 0.0f;
			//右上
            v[1].pos.x = x + size; v[1].pos.y = unitY - size; v[1].u = 1.0f; v[1].v = 0.0f;
            //左下
            v[2].pos.x = x - size; v[2].pos.y = unitY + size; v[2].u = 0.0f; v[2].v = 1.0f;
			//ここは四角形を二つの三角形に分けて描画するため、右上の頂点を再度設定している
            v[3].pos.x = x + size; v[3].pos.y = unitY - size; v[3].u = 1.0f; v[3].v = 0.0f;
			//右下
            v[4].pos.x = x + size; v[4].pos.y = unitY + size; v[4].u = 1.0f; v[4].v = 1.0f;
			//左下を再度設定している
            v[5].pos.x = x - size; v[5].pos.y = unitY + size; v[5].u = 0.0f; v[5].v = 1.0f;

			//プリミティブ(図形)をトライアングルリストとして描画している。6つの頂点を使って2つの三角形を描画することで四角形を表現している
			//4つの点で描画したい場合は、DrawPrimitive2DToShader(v, 4, DX_PRIMTYPE_TRIANGLESTRIP)を使うと良い
            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);    
            //ピクセルシェーダーの終了
            SetUsePixelShader(-1);
			//念のためアルファブレンドモードを元に戻す
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // ==========================================
        // 4. 数値表示（パラパラと減るアニメーション！）
        // ==========================================

		//辞書型、アンオーダードマップを描画用
        static std::unordered_map<const Player*, float> s_displayTotalMap;

		//実際の残機数と数値を取得して、総合パワーを計算する
        int actualStocks = GetStocks();
        int actualNum = GetNumber();
        int actualTotal = actualStocks * 9 + (actualNum - 1); // 残機を含めた総合パワー

        // 初回表示時
		// まだ表示用の数値が存在しない場合は、実際の総合パワーを初期値として設定する
        if (s_displayTotalMap.find(this) == s_displayTotalMap.end()) {
            s_displayTotalMap[this] = (float)actualTotal;
        }

		//参照型で直接アクセスできるようにしている。これにより、s_displayTotalMap[this]の値を直接変更することができる
        float& displayTotal = s_displayTotalMap[this];

        // ゲームリセット時など、数値が離れすぎている場合は一瞬で合わせる
        if (std::abs(displayTotal - actualTotal) > 30.0f) {
            displayTotal = (float)actualTotal;
        }

		//数字の色を符号なし整数型で定義して、初期値は白色に設定している
        unsigned int numColor = GetColor(255, 255, 255); // 基本は白

        // 実際の数値に徐々に近づけていく（パラパラ演出）
        if (displayTotal > actualTotal) {
            displayTotal -= 0.18f; // 減るスピード（少し早め）
            if (displayTotal < actualTotal) displayTotal = (float)actualTotal;
            numColor = GetColor(255, 100, 100); // 減少中は赤く光る！
        }
        else if (displayTotal < actualTotal) {
            displayTotal += 0.18f;
            if (displayTotal > actualTotal) displayTotal = (float)actualTotal;
            numColor = GetColor(100, 255, 150); // 増加中は緑に光る！
        }

        // 表示用の数値を逆算
		//round関数を使って四捨五入して整数に変換しているこれにより、displayTotalが小数点を含む場合でも正確な整数値を得ることができる
        int displayTotalInt = (int)std::round(displayTotal);
		//残機数を9で割った余りを計算して、1を足すことで1~9の範囲に収めている
        int currentNum = (displayTotalInt % 9) + 1;
		//残機数を9で割った商を計算して、残機数を求めている
        if (displayTotalInt < 0) currentNum = 0; // 破壊時

		//to_string関数を使って整数を文字列に変換している
        std::string numStr = std::to_string(currentNum);
		//GetDrawStringWidthToHandle関数を使って文字列の描画幅を取得している。これにより、文字列の幅を計算して中央揃えにすることができる
        int numWidth = GetDrawStringWidthToHandle(numStr.c_str(), (int)numStr.length(), g_playerFontHandle);

        //文字影描画
        DrawStringToHandle((int)x - numWidth / 2 + 2, (int)unitY - 20 + 2, numStr.c_str(), GetColor(20, 10, 0), g_playerFontHandle);
        //文字描画
        DrawStringToHandle((int)x - numWidth / 2, (int)unitY - 20, numStr.c_str(), numColor, g_playerFontHandle); // ★色を適用

        // 5. 演算子バッジ
        //現在の演算子をゲットしてくる
        char currentOp = GetOp();
		//演算子が設定されている場合は
        if (currentOp != '\0') {
			//プレイヤー座標から右に24ピクセル、上に18ピクセルずらした位置にバッジを描画する
            float bx = x + 24.0f;
            float by = unitY + 18.0f;

			//バッジの色を符号なし整数型で定義して、演算子に応じて色を設定している
            unsigned int glowCol, baseCol, edgeCol, shadowCol;
            if (currentOp == '+') { glowCol = GetColor(255, 50, 50); baseCol = GetColor(40, 10, 10); edgeCol = GetColor(255, 100, 100); shadowCol = GetColor(150, 0, 0); }
            else if (currentOp == '-') { glowCol = GetColor(50, 150, 255); baseCol = GetColor(10, 20, 40); edgeCol = GetColor(100, 200, 255); shadowCol = GetColor(0, 50, 150); }
            else if (currentOp == '*') { glowCol = GetColor(50, 255, 100); baseCol = GetColor(10, 40, 15); edgeCol = GetColor(100, 255, 150); shadowCol = GetColor(0, 150, 50); }
            else { glowCol = GetColor(200, 50, 255); baseCol = GetColor(30, 10, 40); edgeCol = GetColor(220, 100, 255); shadowCol = GetColor(100, 0, 150); }

			//バッジの描画
            SetDrawBlendMode(DX_BLENDMODE_ADD, 200); DrawHexagonAA(bx, by, 18.0f, glowCol, TRUE); SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawHexagonAA(bx, by, 15.0f, baseCol, TRUE); DrawHexagonAA(bx, by, 15.0f, edgeCol, FALSE, 2.0f);

			//演算子文字列を描画するために、文字列の長さを1にして、\0で終端する文字列を作成している
            char opStr[2] = { currentOp, '\0' };
            int opW = GetDrawStringWidthToHandle(opStr, 1, g_badgeFontHandle);
			//文字影描画
            DrawStringToHandle((int)bx - opW / 2 + 1, (int)by - 13 + 1, opStr, shadowCol, g_badgeFontHandle);
            //文字描画
            DrawStringToHandle((int)bx - opW / 2, (int)by - 13, opStr, GetColor(255, 255, 255), g_badgeFontHandle);
        }
    }
} // namespace App