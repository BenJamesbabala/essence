
void xpool2_vo(float* v, int* m, float* o, int n, int h, int w, int f) {
    int out_h = h/2;
    int out_w = w/2;
    float* v_ptr = v;
    float* o_ptr = o;
    int* m_ptr = m;
    int v_size = h * w * f;
    int o_size = out_h * out_w * f;
    int step1 = f; int step2 = w*f; int step3 = w*f + f;

    int ni, hi, wi, fi;
    for (ni = 0; ni < n; ++ni){
        for (hi = 0; hi < out_h; ++hi){
            for (wi = 0; wi < out_w; ++wi){
                int off_o = f * (wi + out_w * hi);
                int off_v = f * (2*wi + w * 2*hi);
                for (fi = 0; fi < f; ++fi){
                    int idx_v = off_v + fi;
                    int idx_o = off_o + fi;
                    float max = v_ptr[idx_v];
                    int step = 0;
                    if (v_ptr[idx_v + step1] > max) {
                        max = v_ptr[idx_v + step1];
                        step = step1;
                    }
                    if (v_ptr[idx_v + step2] > max) {
                        max = v_ptr[idx_v + step2];
                        step = step2;
                    }
                    if (v_ptr[idx_v + step3] > max) {
                        max = v_ptr[idx_v + step3];
                        step = step3;
                    }
                    o_ptr[idx_o] = max;
                    m_ptr[idx_o] = step;
                }
            }
        }
        o_ptr += o_size;
        m_ptr += o_size;
        v_ptr += v_size;
    }
}

void xpool2_ov(float* v, int* m, float* o, int n, int h, int w, int f) {
    int out_h = h/2;
    int out_w = w/2;
    float* v_ptr = v;
    float* o_ptr = o;
    int* m_ptr = m;
    int v_size = h * w * f;
    int o_size = out_h * out_w * f;
    int step1 = f; int step2 = w*f; int step3 = w*f + f;

    int ni, hi, wi, fi;
    for (ni = 0; ni < n; ++ni){
        for (hi = 0; hi < out_h; ++hi){
            for (wi = 0; wi < out_w; ++wi){
                int off_o = f * (wi + out_w * hi);
                int off_v = f * (2*wi + w * 2*hi);
                for (fi = 0; fi < f; ++fi){
                    int idx_v = off_v + fi;
                    int idx_o = off_o + fi;
                    v_ptr[idx_v + m_ptr[idx_o]] = o_ptr[idx_o];
                }
            }
        }
        o_ptr += o_size;
        m_ptr += o_size;
        v_ptr += v_size;
    }
}


void conv_vko(float* v, float* k, float* o, 
	int n, int h, int w, int f, 
    int ph, int pw, int out_f, 
	int kh, int kw, int sh, int sw) {

    int out_h = (h + 2*ph - kh) / sh + 1;
    int out_w = (w + 2*pw - kw) / sw + 1;
    int colcol = kh * kw * f;
    int fkw = f * kw;

    int o_size = out_h * out_w * out_f;
    int v_size = h * w * f;
    float* o_ptr = o;
    float* v_ptr = v;

    int n_idx, ho, wo, fo, kcoli;
    int v_hi, v_wi, fi, v_idx, off_o, off_k, sho, swo;
    // im2col + gemm MK x KN = MN
    for (n_idx = 0; n_idx < n; ++n_idx) {
        // im2col + gemm M
        for (ho = 0; ho < out_h; ++ho){
            for (wo = 0; wo < out_w; ++wo){
            	off_o = out_f*(wo+out_w*ho);
            	sho = ho*sh; swo = wo*sw;
                // im2col + gemm K
                for (kcoli = 0; kcoli < colcol; ++kcoli){
                	v_hi = sho + kcoli/fkw % kh;
                	v_wi = swo + kcoli/f % kw;
                    if ((v_hi < ph) | (v_wi < pw) | (v_hi >= h + ph) | (v_wi >= w + pw)) {
                        continue;
                    } 
                    fi = kcoli % f;
                    v_idx = fi + f*(v_wi - pw + w*(v_hi - ph));
                    register float part = v_ptr[v_idx];
                    for (fo = 0; fo < out_f; ++fo) {
                        o_ptr[fo+off_o] += part * k[kcoli*out_f+fo];
                    }
                }
            }
        }
        o_ptr += o_size;
        v_ptr += v_size;
    }
}

void conv_vok(float* v, float* k, float* o, 
	int n, int h, int w, int f, 
    int ph, int pw, int out_f, 
	int kh, int kw, int sh, int sw) {

    int out_h = (h + 2*ph - kh) / sh + 1;
    int out_w = (w + 2*pw - kw) / sw + 1;
    int colcol = kh * kw * f;
    int fkw = f * kw;

    int o_size = out_h * out_w * out_f;
    int v_size = h * w * f;
    float* o_ptr = o;
    float* v_ptr = v;
    register float part;

    int n_idx, ho, wo, fo, kcoli;
    int v_hi, v_wi, fi, v_idx, off_o, off_k, sho, swo;
    for (n_idx = 0; n_idx < n; ++n_idx) {
        for (ho = 0; ho < out_h; ++ho){
            for (wo = 0; wo < out_w; ++wo){
            	off_o = out_f*(wo+out_w*ho);
            	sho = ho*sh; swo = wo*sw;
                for (kcoli = 0; kcoli < colcol; ++kcoli){
                	v_hi = sho + kcoli/fkw % kh;
                	v_wi = swo + kcoli/f % kw;
                    if ((v_hi < ph) | (v_wi < pw) | (v_hi >= h + ph) | (v_wi >= w + pw)) {
                        continue;
                    } 
                    fi = kcoli % f;
                    v_idx = fi + f*(v_wi - pw + w*(v_hi - ph));
                    register float part = v_ptr[v_idx];
                    for (fo = 0; fo < out_f; ++fo) {
                    	k[kcoli*out_f+fo]+=part*o_ptr[fo+off_o];
                    }
                }
            }
        }
        o_ptr += o_size;
        v_ptr += v_size;
    }
}


void conv_kov(float* v, float* k, float* o, 
	int n, int h, int w, int f, 
    int ph, int pw, int out_f, 
	int kh, int kw, int sh, int sw) {

    int out_h = (h + 2*ph - kh) / sh + 1;
    int out_w = (w + 2*pw - kw) / sw + 1;
    int colcol = kh * kw * f;
    int fkw = f * kw;

    int o_size = out_h * out_w * out_f;
    int v_size = h * w * f;
    float* o_ptr = o;
    float* v_ptr = v;

    int n_idx, ho, wo, fo, kcoli;
    int v_hi, v_wi, fi, v_idx, off_o, off_k, sho, swo;
    for (n_idx = 0; n_idx < n; ++n_idx) {
        for (ho = 0; ho < out_h; ++ho){
            for (wo = 0; wo < out_w; ++wo){
            	off_o = out_f*(wo+out_w*ho);
            	sho = ho*sh; swo = wo*sw;
                for (kcoli = 0; kcoli < colcol; ++kcoli){
                	v_hi = sho + kcoli/fkw % kh;
                	v_wi = swo + kcoli/f % kw;
                    if ((v_hi < ph) | (v_wi < pw) | (v_hi >= h + ph) | (v_wi >= w + pw)) {
                        continue;
                    } 
                    fi = kcoli % f;
                    v_idx = fi + f*(v_wi - pw + w*(v_hi - ph));
                    register float part = 0;
                    for (fo = 0; fo < out_f; ++fo) {
                    	part +=	k[kcoli*out_f+fo]*o_ptr[fo+off_o];
                    }
                    v_ptr[v_idx] += part;
                }
            }
        }
        o_ptr += o_size;
        v_ptr += v_size;
    }
}