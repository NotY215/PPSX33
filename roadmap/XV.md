# Phase XV — Real rendering

## Status
- Seed frame (bars + triangle) **stable** (user screenshot confirmed)
- Still **not** game geometry — inject/seed only
- Log issues fixed in XV-I: multi-draw spam, cumulative packets, inject flood

## XV-I
- One seed composition per flip
- Inject ≤ once per flip
- Honest methods/packets per frame after flip log
- Control GET synced from RSX after flush

## Next (real title pixels)
1. Resolve sticky BREAK / unknown PRX NIDs so boot advances
2. Real GCM PUT movement from game
3. Disable seed when `methods` from non-inject stream > threshold
