# serial port tool

# 生成 ts 文件
```powershell
lupdate -recursive . -ts translations/zh_CN.ts translations/en_US.ts
```

# 把ts文件变成 qm
```
lrelease translations/zh_CN.ts translations/en_US.ts
```