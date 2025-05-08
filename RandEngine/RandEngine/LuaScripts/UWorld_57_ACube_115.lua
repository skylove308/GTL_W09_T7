function BeginPlay()
    print("Begin")
    -- UE_LOG("Error", "actor:SetLocation")
end

function EndPlay()
    print("[EndPlay] ")
    -- obj:PrintLocation()
end

function OnOverlap(OtherActor)
    -- OtherActor:PrintLocation();
end

function Tick(dt)
    local currentPos = actor.Location
    currentPos.X = currentPos.X + dt * 1
    actor.Location = currentPos
    -- actor:SetLocation(newPos)
    -- actor:SetLocation(FVector(200, 0, 0))
    -- UE_LOG("Error", "actor:SetLocation")

    -- actor.Location = FVector(0, 0, 0)
    -- actor.Location.X = actor.Location.X + dt
end

function BeginOverlap()
end

function EndOverlap()
end
